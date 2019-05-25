
#include "client.h"
#include "msgDef.h"
#include "postgres.h"
#include "catalog/pg_type.h"
#include "catalog/pg_operator.h"
#include "commands/defrem.h"
#include "commands/explain.h"
#include "commands/vacuum.h"
#include "foreign/fdwapi.h"
#include "foreign/foreign.h"
#include "sdb_fdw_util.h"

#include "access/htup_details.h"
#include "access/reloptions.h"
#include "access/xact.h"
#include "catalog/pg_type.h"
#include "catalog/pg_operator.h"
#include "commands/defrem.h"
#include "commands/explain.h"
#include "commands/vacuum.h"
#include "foreign/fdwapi.h"
#include "foreign/foreign.h"
#include "nodes/makefuncs.h"
#include "nodes/relation.h"
#include "optimizer/cost.h"
#include "optimizer/clauses.h"
#include "optimizer/pathnode.h"
#include "optimizer/plancat.h"
#include "optimizer/planmain.h"
#include "optimizer/restrictinfo.h"
#include "optimizer/var.h"
#include "parser/parsetree.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/date.h"
#include "utils/hsearch.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"
#include "utils/memutils.h"
#include "utils/numeric.h"
#include "utils/rel.h"
#include "utils/timestamp.h"

/* Callback argument for ec_member_matches_indexcol */
typedef struct
{
   IndexOptInfo *index;    /* index we're considering */
   int         indexcol;      /* index column we want to match to */
} sdb_ec_member_matches_arg;


AppendRelInfo * sdb_find_childrel_appendrelinfo(PlannerInfo *root,
                                                RelOptInfo *rel) ;

static Oid sdb_select_equality_operator(EquivalenceClass *ec, Oid lefttype,
                                 Oid righttype) ;

static RestrictInfo *sdb_create_join_clause(PlannerInfo *root,
               EquivalenceClass *ec, Oid opno,
               EquivalenceMember *leftem,
               EquivalenceMember *rightem,
               EquivalenceClass *parent_ec) ;

static List *sdbGenerateImpliedEqualitiesForColumn(PlannerInfo *root,
                              RelOptInfo *rel, Oid tableID,
                              sdbIndexInfo *indexInfo, int indexcol,
                              Relids prohibited_rels) ;

static bool sdbMatchColumn(RelOptInfo *rel, Oid foreignID, Node *operand,
                           sdbIndexInfo *indexInfo, int indexcol) ;

static bool sdbIsVarNode( Node *node ) ;

static double sdb_get_loop_count(PlannerInfo *root, Relids outer_relids) ;

static void sdb_expand_indexqual_conditions(List *indexclauses,
                     List *indexclausecols, List **indexquals_p,
                     List **indexqualcols_p) ;

static int sdbGetIndexInfosFromDB( SdbExecState *sdbState, sdbIndexInfo *indexInfo,
                                   INT32 maxNum, INT32 *indexNum ) ;

void debugArgumentInfo( Oid tableID, Expr *node )
{
   if( nodeTag(node) == T_Var )
   {
      Var *var   = (Var *)node ;
      char *name = get_relid_attribute_name( tableID, var->varattno ) ;
      elog(DEBUG1, "var(name=%s)", name) ;
   }
   else if ( nodeTag(node) == T_Param )
   {
      elog(DEBUG1, "param(name)") ;
   }
   else if ( nodeTag(node) == T_Const )
   {
      sdbbson obj;
      Const *con = (Const *)node ;

      sdbbson_init(&obj) ;
      sdbSetBsonValue( &obj, "const", con->constvalue,
                       con->consttype, con->consttypmod, 0 ) ;

      elog(DEBUG1, "const()");
      sdbPrintBson( &obj, DEBUG1, "Const" ) ;
      sdbbson_destroy(&obj);
   }
   else
   {
      elog(DEBUG1, "unknown NodeType:type=%d", nodeTag( node )) ;
   }
}

void debugEqClause( PlannerInfo *root )
{

}

void debugRestrictInfo(RestrictInfo *restrictInfo, Oid tableID)
{
   Expr *expression           = restrictInfo->clause ;
   NodeTag expressionType     = 0 ;
   OpExpr *opExpression       = NULL ;
   CHAR *operatorName         = NULL ;
   List *argumentList         = NIL ;
   ListCell *argumentCell     = NULL ;

   /* we only support operator expressions */
   expressionType = nodeTag( expression ) ;
   if( T_OpExpr != expressionType )
   {
      return ;
   }
   opExpression = ( OpExpr* )expression ;
   operatorName = get_opname( opExpression->opno ) ;
   elog(DEBUG1, "******operation=%s******", operatorName);
   /* we only support simple binary operators */
   argumentList = opExpression->args ;
   foreach( argumentCell, argumentList )
   {
      Expr *argument = ( Expr * )lfirst( argumentCell ) ;

      if ( nodeTag( argument ) == T_RelabelType )
      {
         RelabelType *relabel = (RelabelType *)argument ;
         elog(DEBUG1, "relabel*************************");
         debugArgumentInfo(tableID, relabel->arg);
         elog(DEBUG1, "relabel end*********************");
      }
      else
      {
         debugArgumentInfo(tableID, argument) ;
      }
   }
}

void debugClauseInfo( PlannerInfo *root, RelOptInfo *baserel, Oid tableID )
{
   List *restrictInfoList     = baserel->baserestrictinfo ;
   ListCell *restrictInfoCell = NULL ;

   elog(DEBUG1, "*****baserel->baserestrictinfo begin********************");
   foreach( restrictInfoCell, restrictInfoList )
   {
      RestrictInfo *restrictInfo = ( RestrictInfo * )lfirst( restrictInfoCell ) ;
      debugRestrictInfo(restrictInfo, tableID) ;
   }
   elog(DEBUG1, "*****baserel->baserestrictinfo end********************");

   elog(DEBUG1, "*****root->eq_classes begin********************");
   debugEqClause(root);
   elog(DEBUG1, "*****root->eq_classes end********************");
}

#ifdef SDB_USE_OWN_POSTGRES

void sdbuseownpostgres()
{
   elog(DEBUG1, "use own postgres");
}

#endif /* SDB_USE_OWN_POSTGRES */

void sdbGetIndexEqclause( PlannerInfo *root, RelOptInfo *baserel, Oid tableID,
                          sdbIndexInfo *indexInfo,
                          sdbIndexClauseSet *clauseset )
{
   int indexcol;

   if ( !baserel->has_eclass_joins )
   {
      return ;
   }

   for (indexcol = 0; indexcol < indexInfo->keyNum; indexcol++)
   {
      sdb_ec_member_matches_arg arg;
      List     *clauses;

      /* Generate clauses, skipping any that join to lateral_referencers */
      arg.index = NULL;
      arg.indexcol = indexcol;
      clauses = sdbGenerateImpliedEqualitiesForColumn(root, baserel, tableID,
                                                 indexInfo, indexcol,
                                                 baserel->lateral_referencers);

      if ( clauses )
      {
         ListCell *lc ;
         foreach(lc, clauses)
         {
            RestrictInfo *rinfo = (RestrictInfo *) lfirst(lc);

            Assert(IsA(rinfo, RestrictInfo));
            clauseset->indexclauses[indexcol] =
            list_append_unique_ptr(clauseset->indexclauses[indexcol],
                              rinfo);
            clauseset->nonempty = true;
         }
      }
   }
}

void sdbMatchJoinClausesToIndex( PlannerInfo *root, RelOptInfo *rel,
                                 Oid tableID, sdbIndexInfo *index,
                                 sdbIndexClauseSet *clauseset )
{
   ListCell   *lc;

   /* Scan the rel's join clauses */
   foreach(lc, rel->joininfo)
   {
      RestrictInfo *rinfo = (RestrictInfo *) lfirst(lc);

      /* Check if clause can be moved to this rel */
      if (!join_clause_is_movable_to(rinfo, rel))
         continue;

      /* Potentially usable, so see if it matches the index or is an OR */
      if (!restriction_is_or_clause(rinfo))
      {
         sdbMatchClauseToIndex(rel, tableID, index, rinfo, clauseset);
      }
   }
}

void sdbMatchClauseToIndex( RelOptInfo *rel, Oid tableID, sdbIndexInfo *index,
                            RestrictInfo *rinfo, sdbIndexClauseSet *clauseset )
{
   int indexcol ;

   for ( indexcol = 0 ; indexcol < index->keyNum ; indexcol++ )
   {
      if ( sdbMatchClauseToIndexcol(rel, tableID, index, indexcol, rinfo) )
      {
         clauseset->indexclauses[indexcol] =
            list_append_unique_ptr(clauseset->indexclauses[indexcol], rinfo) ;
         clauseset->nonempty = true ;
         return ;
      }
   }
}

bool sdbIsVarNode( Node *node )
{
   if ( IsA(node, RelabelType) )
   {
      node = (Node *) ((RelabelType *) node)->arg ;
   }

   if ( IsA(node, Var) )
   {
      return true ;
   }

   return false ;
}

bool sdbMatchClauseToIndexcol( RelOptInfo *rel, Oid tableID,
                               sdbIndexInfo *index, int indexcol,
                               RestrictInfo *rinfo )
{
   Expr *clause      = rinfo->clause;
   Index index_relid = rel->relid;
   Node *leftop      = NULL ;
   Node *rightop     = NULL ;
   Relids left_relids ;
   Relids right_relids ;

   if ( rinfo->pseudoconstant )
   {
      return false ;
   }

   if ( is_opclause(clause) )
   {
      leftop  = get_leftop(clause) ;
      rightop = get_rightop(clause) ;
      if ( !leftop || !rightop )
      {
         return false ;
      }

      left_relids  = rinfo->left_relids ;
      right_relids = rinfo->right_relids ;
   }
   else
   {
      return false;
   }

   if ( !sdbIsVarNode( leftop ) || !sdbIsVarNode( rightop ) )
   {
      return false ;
   }

   if ( sdbMatchColumn( rel, tableID, leftop, index, indexcol ) &&
        !bms_is_member( index_relid, right_relids ) )
   {
      return true ;
   }

   if ( sdbMatchColumn( rel, tableID, rightop, index, indexcol ) &&
        !bms_is_member( index_relid, left_relids ) )
   {
      return true ;
   }

   return false;
}

List *sdbGenerateImpliedEqualitiesForColumn(PlannerInfo *root,
                              RelOptInfo *rel,
                              Oid tableID,
                              sdbIndexInfo *indexInfo, int indexcol,
                              Relids prohibited_rels)
{
   List     *result = NIL;
   bool     is_child_rel = (rel->reloptkind == RELOPT_OTHER_MEMBER_REL);
   Index    parent_relid;
   ListCell *lc1;

   /* If it's a child rel, we'll need to know what its parent is */
   if (is_child_rel)
      parent_relid = sdb_find_childrel_appendrelinfo(root, rel)->parent_relid;
   else
      parent_relid = 0;    /* not used, but keep compiler quiet */

   foreach(lc1, root->eq_classes)
   {
      EquivalenceClass *cur_ec = (EquivalenceClass *) lfirst(lc1);
      EquivalenceMember *cur_em;
      ListCell   *lc2;

      if (cur_ec->ec_has_const || list_length(cur_ec->ec_members) <= 1)
         continue;

      if (!is_child_rel &&
         !bms_is_subset(rel->relids, cur_ec->ec_relids))
         continue;

      cur_em = NULL;
      foreach(lc2, cur_ec->ec_members)
      {
         cur_em = (EquivalenceMember *) lfirst(lc2);
         if (bms_equal(cur_em->em_relids, rel->relids) &&
             sdbMatchColumn(rel, tableID, (Node *) cur_em->em_expr,
                            indexInfo, indexcol))
            break;
         cur_em = NULL;
      }

      if (!cur_em)
         continue;

      foreach(lc2, cur_ec->ec_members)
      {
         EquivalenceMember *other_em = (EquivalenceMember *) lfirst(lc2);
         Oid         eq_op;
         RestrictInfo *rinfo;

         if (other_em->em_is_child)
            continue;      /* ignore children here */

         if (other_em == cur_em ||
            bms_overlap(other_em->em_relids, rel->relids))
            continue;

         if (bms_overlap(other_em->em_relids, rel->lateral_referencers))
            continue;

         if (is_child_rel &&
            bms_is_member(parent_relid, other_em->em_relids))
            continue;

         eq_op = sdb_select_equality_operator(cur_ec,
                                  cur_em->em_datatype,
                                  other_em->em_datatype);
         if (!OidIsValid(eq_op))
            continue;

         rinfo = sdb_create_join_clause(root, cur_ec, eq_op,
                              cur_em, other_em,
                              cur_ec);

         result = lappend(result, rinfo);
      }

      if (result)
         break;
   }

   return result;
}

bool sdbMatchColumn(RelOptInfo *rel, Oid foreignID, Node *operand,
                    sdbIndexInfo *indexInfo, int indexcol)
{
   if (operand && IsA(operand, RelabelType))
   {
      operand = (Node *) ((RelabelType *) operand)->arg;
   }

   if (operand && IsA(operand, Var))
   {
      Var *var = (Var *) operand;
      if (rel->relid == var->varno && var->varlevelsup == 0)
      {
         AttrNumber columnId = var->varattno ;
         char *columnName    = get_relid_attribute_name( foreignID, columnId ) ;
         if (strcmp(indexInfo->indexKey[indexcol], columnName) == 0)
         {
            return true;
         }
      }
   }

   return false ;
}

RestrictInfo *sdb_create_join_clause(PlannerInfo *root,
               EquivalenceClass *ec, Oid opno,
               EquivalenceMember *leftem,
               EquivalenceMember *rightem,
               EquivalenceClass *parent_ec)
{
   RestrictInfo *rinfo;
   ListCell   *lc;
   MemoryContext oldcontext;

   /*
    * Search to see if we already built a RestrictInfo for this pair of
    * EquivalenceMembers.  We can use either original source clauses or
    * previously-derived clauses.   The check on opno is probably redundant,
    * but be safe ...
    */
   foreach(lc, ec->ec_sources)
   {
      rinfo = (RestrictInfo *) lfirst(lc);
      if (rinfo->left_em == leftem &&
         rinfo->right_em == rightem &&
         rinfo->parent_ec == parent_ec &&
         opno == ((OpExpr *) rinfo->clause)->opno)
         return rinfo;
   }

   foreach(lc, ec->ec_derives)
   {
      rinfo = (RestrictInfo *) lfirst(lc);
      if (rinfo->left_em == leftem &&
         rinfo->right_em == rightem &&
         rinfo->parent_ec == parent_ec &&
         opno == ((OpExpr *) rinfo->clause)->opno)
         return rinfo;
   }

   /*
    * Not there, so build it, in planner context so we can re-use it. (Not
    * important in normal planning, but definitely so in GEQO.)
    */
   oldcontext = MemoryContextSwitchTo(root->planner_cxt);

   rinfo = build_implied_join_equality(opno,
                              ec->ec_collation,
                              leftem->em_expr,
                              rightem->em_expr,
                              bms_union(leftem->em_relids,
                                      rightem->em_relids),
                              bms_union(leftem->em_nullable_relids,
                                    rightem->em_nullable_relids));

   /* Mark the clause as redundant, or not */
   rinfo->parent_ec = parent_ec;

   /*
    * We know the correct values for left_ec/right_ec, ie this particular EC,
    * so we can just set them directly instead of forcing another lookup.
    */
   rinfo->left_ec = ec;
   rinfo->right_ec = ec;

   /* Mark it as usable with these EMs */
   rinfo->left_em = leftem;
   rinfo->right_em = rightem;
   /* and save it for possible re-use */
   ec->ec_derives = lappend(ec->ec_derives, rinfo);

   MemoryContextSwitchTo(oldcontext);

   return rinfo;
}

AppendRelInfo * sdb_find_childrel_appendrelinfo(PlannerInfo *root,
                                                RelOptInfo *rel)
{
   Index    relid = rel->relid;
   ListCell   *lc;

   /* Should only be called on child rels */
   Assert(rel->reloptkind == RELOPT_OTHER_MEMBER_REL);

   foreach(lc, root->append_rel_list)
   {
      AppendRelInfo *appinfo = (AppendRelInfo *) lfirst(lc);

      if (appinfo->child_relid == relid)
         return appinfo;
   }
   /* should have found the entry ... */
   elog(ERROR, "child rel %d not found in append_rel_list", relid);
   return NULL;            /* not reached */
}


Oid sdb_select_equality_operator(EquivalenceClass *ec, Oid lefttype,
                                 Oid righttype)
{
   ListCell   *lc;

   foreach(lc, ec->ec_opfamilies)
   {
      Oid         opfamily = lfirst_oid(lc);
      Oid         opno;

      opno = get_opfamily_member(opfamily, lefttype, righttype,
                           BTEqualStrategyNumber);
      if (OidIsValid(opno))
         return opno;
   }
   return InvalidOid;
}



/*int sdbGetIndexInfo( SdbExecState *sdbState, sdbIndexInfo *indexInfo )
{
   INT32 rc = SDB_OK ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   sdbbson obj ;

   sdbState->hConnection = sdbGetConnectionHandle(
                                        (const char **)sdbState->sdbServerList,
                                        sdbState->sdbServerNum,
                                        sdbState->usr,
                                        sdbState->passwd,
                                        sdbState->preferenceInstance,
                                        sdbState->transaction ) ;
   sdbState->hCollection = sdbGetSdbCollection(sdbState->hConnection,
                           sdbState->sdbcs, sdbState->sdbcl) ;

   rc = sdbGetIndexes(sdbState->hCollection, NULL, &cursor) ;
   if ( SDB_OK != rc )
   {
      ereport(WARNING, (errcode(ERRCODE_FDW_INVALID_OPTION_INDEX),
            errmsg("Cannot get sdb's indexinfo"), errhint(" "))) ;
      goto error ;
   }

   sdbbson_init(&obj);
   while(!(rc=sdbNext(cursor, &obj)))
   {
      sdbbson_iterator sdbbsonIter1 = {NULL, 0} ;
      sdbbson_iterator_init(&sdbbsonIter1, &obj) ;

      while(sdbbson_iterator_next(&sdbbsonIter1))
      {
         const CHAR *sdbbsonKey   = sdbbson_iterator_key(&sdbbsonIter1) ;
         sdbbson_type sdbbsonType = sdbbson_iterator_type(&sdbbsonIter1) ;
         if ( strcmp(sdbbsonKey, "IndexDef") == 0
              && sdbbsonType == BSON_OBJECT )
         {
            sdbbson indexDef ;
            sdbbson_iterator sdbbsonIter2 = {NULL, 0} ;
            sdbbson_init(&indexDef);
            sdbbson_iterator_subobject(&sdbbsonIter1, &indexDef) ;
            sdbbson_iterator_init(&sdbbsonIter2, &indexDef) ;
            while( sdbbson_iterator_next(&sdbbsonIter2) )
            {
               const CHAR *key2   = sdbbson_iterator_key(&sdbbsonIter2) ;
               sdbbson_type type2 = sdbbson_iterator_type(&sdbbsonIter2) ;
               if ( strcmp(key2, "key") == 0 && type2 == BSON_OBJECT )
               {
                  sdbbson keyDef ;
                  sdbbson_iterator sdbbsonIter3 = {NULL, 0} ;
                  int i = 0 ;
                  sdbbson_init(&keyDef);
                  sdbbson_iterator_subobject(&sdbbsonIter2, &keyDef) ;
                  sdbbson_iterator_init(&sdbbsonIter3, &keyDef) ;
                  while ( sdbbson_iterator_next(&sdbbsonIter3) )
                  {
                     const CHAR *key3 = sdbbson_iterator_key(&sdbbsonIter3) ;
                     if ( strcmp(key3, "_id") != 0 )
                     {
                        strncpy( indexInfo->indexKey[i], key3,
                              SDB_MAX_KEY_COLUMN_LENGTH ) ;
                        indexInfo->indexKey[i][SDB_MAX_KEY_COLUMN_LENGTH-1] = 0 ;
                        i++ ;
                     }
                  }

                  sdbbson_destroy(&keyDef) ;

                  indexInfo->keyNum = i ;
                  if ( indexInfo->keyNum > 0 )
                  {
                     sdbbson_destroy(&indexDef) ;
                     goto done ;
                  }
               }
            }
            sdbbson_destroy(&indexDef) ;
         }
      }

      sdbbson_destroy(&obj) ;
      sdbbson_init(&obj);
   }

done:
   sdbbson_destroy(&obj) ;
   if ( SDB_INVALID_HANDLE != sdbState->hCollection )
   {
      sdbReleaseCollection(sdbState->hCollection) ;
      sdbState->hCollection = SDB_INVALID_HANDLE ;
   }

   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor(cursor);
   }

   return rc ;
error:
   goto done ;
}
*/

int sdbGetIndexInfosFromCache( SdbCLStatistics *clCache, sdbIndexInfo *indexInfo,
                               INT32 maxNum, INT32 *indexNum )
{
   int i = 0;
   for ( i = 0; ( i < clCache->indexNum && i < maxNum ); i++ )
   {
      memcpy( &indexInfo[i], &clCache->indexInfo[i], sizeof(sdbIndexInfo) ) ;
      elog( DEBUG1, "i=%d,keynum=%d,key[0]=%s", i, indexInfo[i].keyNum,
            indexInfo[i].indexKey[0] ) ;
   }

   *indexNum = i ;
   return SDB_OK ;
}

int sdbGetIndexInfos( SdbExecState *sdbState, sdbIndexInfo *indexInfo,
                      INT32 maxNum, INT32 *indexNum )
{
   INT32 rc = SDB_OK ;
   SdbStatisticsCache *cache = NULL ;
   SdbCLStatistics *clCache = NULL ;

   cache = SdbGetStatisticsCache() ;
   clCache = SdbGetCLStatFromCache( sdbState->tableID ) ;
   if ( clCache->indexNum >= 0 )
   {
      elog( DEBUG1, "get index from cache" ) ;
      rc = sdbGetIndexInfosFromCache( clCache, indexInfo, maxNum, indexNum ) ;
   }
   else
   {
      elog( DEBUG1, "get index from db" ) ;
      rc = sdbGetIndexInfosFromDB( sdbState, indexInfo, maxNum, indexNum ) ;
      if ( SDB_OK == rc )
      {
         int i = 0;
         clCache->indexNum = *indexNum ;
         for ( i = 0; i < clCache->indexNum; i++ )
         {
            memcpy( &clCache->indexInfo[i], &indexInfo[i], sizeof(sdbIndexInfo) ) ;
         }
      }
   }

   return rc ;
}


int sdbGetIndexInfosFromDB( SdbExecState *sdbState, sdbIndexInfo *indexInfo,
                            INT32 maxNum, INT32 *indexNum )
{
   INT32 rc = SDB_OK ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   sdbbson obj ;
   INT32 count = 0 ;

   sdbState->hConnection = sdbGetConnectionHandle(
                                        (const char **)sdbState->sdbServerList,
                                        sdbState->sdbServerNum,
                                        sdbState->usr,
                                        sdbState->passwd,
                                        sdbState->preferenceInstance,
                                        sdbState->transaction ) ;
   sdbState->hCollection = sdbGetSdbCollection(sdbState->hConnection,
                           sdbState->sdbcs, sdbState->sdbcl) ;

   rc = sdbGetIndexes(sdbState->hCollection, NULL, &cursor) ;
   if ( SDB_OK != rc )
   {
      elog( WARNING, "Cannot get sdb's indexinfo:rc=%d", rc ) ;
      goto error ;
   }

   sdbbson_init(&obj);
   while(!(rc=sdbNext(cursor, &obj)))
   {
      sdbbson_iterator sdbbsonIter1 = {NULL, 0} ;
      sdbbson_iterator_init(&sdbbsonIter1, &obj) ;

      /* for each element in sdbbson object */
      while(sdbbson_iterator_next(&sdbbsonIter1))
      {
         const CHAR *sdbbsonKey   = sdbbson_iterator_key(&sdbbsonIter1) ;
         sdbbson_type sdbbsonType = sdbbson_iterator_type(&sdbbsonIter1) ;
         if ( strcmp(sdbbsonKey, "IndexDef") == 0
              && sdbbsonType == BSON_OBJECT )
         {
            sdbbson indexDef ;
            sdbbson_iterator sdbbsonIter2 = {NULL, 0} ;
            sdbbson_init(&indexDef);
            sdbbson_iterator_subobject(&sdbbsonIter1, &indexDef) ;
            sdbbson_iterator_init(&sdbbsonIter2, &indexDef) ;
            while( sdbbson_iterator_next(&sdbbsonIter2) )
            {
               const CHAR *key2   = sdbbson_iterator_key(&sdbbsonIter2) ;
               sdbbson_type type2 = sdbbson_iterator_type(&sdbbsonIter2) ;
               if ( strcmp(key2, "key") == 0 && type2 == BSON_OBJECT )
               {
                  sdbbson keyDef ;
                  sdbbson_iterator sdbbsonIter3 = {NULL, 0} ;
                  int i = 0 ;
                  sdbbson_init(&keyDef);
                  sdbbson_iterator_subobject(&sdbbsonIter2, &keyDef) ;
                  sdbbson_iterator_init(&sdbbsonIter3, &keyDef) ;
                  while ( sdbbson_iterator_next(&sdbbsonIter3) )
                  {
                     const CHAR *key3 = sdbbson_iterator_key(&sdbbsonIter3) ;
                     if ( strcmp(key3, "_id") != 0 )
                     {
                        strncpy( indexInfo[count].indexKey[i], key3,
                              SDB_MAX_KEY_COLUMN_LENGTH ) ;
                        indexInfo[count].indexKey[i][SDB_MAX_KEY_COLUMN_LENGTH-1] = 0 ;
                        i++ ;
                     }
                  }

                  sdbbson_destroy(&keyDef) ;

                  indexInfo[count].keyNum = i ;
                  if ( indexInfo[count].keyNum > 0 )
                  {
                     count++;
                     if ( count >= maxNum )
                     {
                        sdbbson_destroy(&indexDef) ;
                        goto done ;
                     }
                  }
               }
            }
            sdbbson_destroy(&indexDef) ;
         }
      }

      sdbbson_destroy(&obj) ;
      sdbbson_init(&obj);
   }

   if ( rc == SDB_DMS_EOC )
   {
      rc = SDB_OK ;
   }

done:
   *indexNum = count ;
   sdbbson_destroy(&obj) ;
   if ( SDB_INVALID_HANDLE != sdbState->hCollection )
   {
      sdbReleaseCollection(sdbState->hCollection) ;
      sdbState->hCollection = SDB_INVALID_HANDLE ;
   }

   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor(cursor);
   }

   return rc ;
error:
   goto done ;
}


extern bool QueryCancelPending ;

BOOLEAN sdbIsInterrupt()
{
   if ( QueryCancelPending )
   {
      return TRUE ;
   }

   return FALSE ;
}

sdbConnectionHandle sdbGetConnectionHandle( const char **serverList,
                                            int serverNum,
                                            const char *usr,
                                            const char *passwd,
                                            const char *preference_instance,
                                            const char *transaction )
{
   sdbConnectionHandle hConnection = SDB_INVALID_HANDLE ;
   SdbConnectionPool *pool         = NULL ;
   INT32 count                     = 0 ;
   INT32 rc                        = SDB_OK ;
   INT32 i                         = 0 ;
   SdbConnection *connect          = NULL ;

   /* connection string is address + service + user + password */
   StringInfo connName = makeStringInfo() ;
   i = 0 ;
   while ( i < serverNum )
   {
      appendStringInfo( connName, "%s:", serverList[i] ) ;
      i++ ;
   }
   appendStringInfo( connName, "%s:%s", usr, passwd ) ;

   /* iterate all connections in pool */
   pool = sdbGetConnectionPool() ;
   for ( count = 0 ; count < pool->numConnections ; ++count )
   {
      SdbConnection *tmpConnection = &pool->connList[count] ;
      if ( strcmp( tmpConnection->connName, connName->data ) == 0 )
      {
         BOOLEAN result = FALSE ;
         sdbIsValid( tmpConnection->hConnection, &result ) ;
         if ( !result )
         {
            sdbReleaseConnectionFromPool( count ) ;
            break ;
         }

         if ( 1 == tmpConnection->isTransactionOn )
         {
            if ( tmpConnection->transLevel <= 0 )
            {
               tmpConnection->transLevel = 1 ;
               elog( DEBUG1, "trans begin[%s]", tmpConnection->connName ) ;
               rc = sdbTransactionBegin( tmpConnection->hConnection ) ;
               if ( SDB_OK != rc )
               {
                  ereport( ERROR, ( errcode( ERRCODE_FDW_ERROR ),
                           errmsg( "begin transaction failed:rc=%d", rc ) ) ) ;
                  return SDB_INVALID_HANDLE ;
               }
            }
         }

         return tmpConnection->hConnection;
      }
   }

   /* when we get here, we don't have the connection so let's create one */
   ereport( DEBUG1, (errcode( ERRCODE_FDW_UNABLE_TO_ESTABLISH_CONNECTION ),
            errmsg( "connecting :list=%s,num=%d", connName->data, serverNum ) )) ;
   rc = sdbConnect1( serverList, serverNum, usr, passwd, &hConnection ) ;
   if ( rc )
   {
      StringInfo tmpInfo = makeStringInfo() ;
      i = 0 ;
      while ( i < serverNum )
      {
         appendStringInfo( tmpInfo, "%s:", serverList[i] ) ;
         i++ ;
      }
      ereport( ERROR, ( errcode( ERRCODE_FDW_UNABLE_TO_ESTABLISH_CONNECTION ),
                        errmsg( "unable to establish connection to \"%s\""
                                ", rc = %d", tmpInfo->data, rc ),
                        errhint( "Make sure remote service is running "
                                 "and username/password are valid" ) ) ) ;
      return SDB_INVALID_HANDLE ;
   }

   rc = sdbSetConnectionPreference( hConnection, preference_instance ) ;
   if ( rc )
   {
      ereport( WARNING, ( errcode( ERRCODE_WITH_CHECK_OPTION_VIOLATION ),
                          errmsg( "set connection's preference instance failed"
                                  ":rc=%d,preference=%s", rc,
                                  preference_instance ),
                          errhint( "Make sure the OPTION_NAME_PREFEREDINSTANCE "
                                   "are valid" ) ) ) ;
   }

   sdbSetConnectionInterruptFunc( hConnection, sdbIsInterrupt ) ;

   /* add connection into pool */
   if ( pool->poolSize <= pool->numConnections )
   {
      /* allocate new slots */
      SdbConnection *pNewMem = pool->connList ;
      INT32 poolSize = pool->poolSize ;
      poolSize = poolSize << 1 ;
      pNewMem  = ( SdbConnection* )realloc( pNewMem, sizeof( SdbConnection )* poolSize ) ;
      if ( !pNewMem )
      {
         sdbDisconnect( hConnection ) ;
         ereport( ERROR, ( errcode( ERRCODE_FDW_OUT_OF_MEMORY ),
                           errmsg( "Unable to allocate connection pool" ),
                           errhint( "Make sure the memory pool or ulimit is "
                                    "properly configured" ) ) ) ;
         return SDB_INVALID_HANDLE ;
      }

      pool->connList = pNewMem ;
      pool->poolSize = poolSize ;
   }

   connect = &pool->connList[pool->numConnections] ;
   connect->connName      = strdup( connName->data ) ;
   connect->hConnection   = hConnection ;
   connect->isTransactionOn = 0 ;
   pool->numConnections++ ;

   if ( strcmp( transaction, SDB_TRANSACTION_ON ) == 0 )
   {
      elog( DEBUG1, "trans begin[%s]", connect->connName ) ;
      rc = sdbTransactionBegin( connect->hConnection ) ;
      if ( SDB_OK != rc )
      {
         ereport( ERROR, ( errcode( ERRCODE_FDW_ERROR ),
                  errmsg( "begin transaction failed:rc=%d", rc ) ) ) ;
         return SDB_INVALID_HANDLE ;
      }

      connect->isTransactionOn = 1 ;
      connect->transLevel      = 1 ;
   }

   return hConnection ;
}

sdbCollectionHandle sdbGetSdbCollection( sdbConnectionHandle connectionHandle,
      const char *sdbcs, const char *sdbcl )

{
   /* get collection */
   int rc = SDB_OK ;
   sdbCollectionHandle hCollection = SDB_INVALID_HANDLE ;
   StringInfo fullCollectionName   = makeStringInfo(  ) ;
   appendStringInfoString( fullCollectionName, sdbcs ) ;
   appendStringInfoString( fullCollectionName, "." ) ;
   appendStringInfoString( fullCollectionName, sdbcl ) ;
   rc = sdbGetCollection( connectionHandle, fullCollectionName->data, &hCollection ) ;
   if ( rc )
   {
      ereport( ERROR, ( errcode( ERRCODE_FDW_ERROR ),
                         errmsg( "Unable to get collection \"%s\", rc = %d",
                                  fullCollectionName->data, rc ),
                         errhint( "Make sure the collectionspace and "
                                   "collection exist on the remote database" ) ) ) ;
   }

   return hCollection ;
}

void sdbReleaseConnectionFromPool( int index )
{
   INT32 i = index ;
   SdbConnection *connection = NULL ;
   INT32 j = i + 1 ;

   SdbConnectionPool *pool = sdbGetConnectionPool() ;
   if ( i >= pool->numConnections )
   {
      return ;
   }

   connection = &pool->connList[i] ;
   if( connection->connName )
   {
      free( connection->connName ) ;
   }
   sdbDisconnect( connection->hConnection ) ;
   sdbReleaseConnection( connection->hConnection ) ;

   for ( ; j < pool->numConnections ; ++i, ++j )
   {
      pool->connList[i] = pool->connList[j] ;
   }

   pool->numConnections-- ;
}

int sdbSetConnectionPreference( sdbConnectionHandle hConnection,
   const CHAR *preference_instance )
{
   int intPreferenece_instance = 0 ;
   int rc = 0 ;
   if ( NULL != preference_instance ){
      sdbbson recordObj ;
      sdbbson_init( &recordObj ) ;
      intPreferenece_instance = atoi( preference_instance ) ;
      if ( 0 == intPreferenece_instance )
      {
         sdbbson_append_string( &recordObj, FIELD_NAME_PREFERED_INSTANCE,
                                preference_instance ) ;
      }
      else
      {
         sdbbson_append_int( &recordObj, FIELD_NAME_PREFERED_INSTANCE,
                             intPreferenece_instance ) ;
      }

      rc = sdbbson_finish( &recordObj ) ;
      if ( rc != SDB_OK )
      {
         ereport( WARNING, ( errcode( ERRCODE_FDW_ERROR ),
               errmsg( "finish bson failed:rc = %d", rc ),
               errhint( "Make sure the data is all right" ) ) ) ;

         sdbbson_destroy( &recordObj ) ;
         return rc ;
      }

      rc = sdbSetSessionAttr( hConnection, &recordObj ) ;
      if ( rc != SDB_OK )
      {
         ereport( WARNING, ( errcode( ERRCODE_FDW_ERROR ),
               errmsg( "set session attribute failed:rc = %d", rc ),
               errhint( "Make sure the session is all right" ) ) ) ;

         sdbbson_destroy( &recordObj ) ;
         return rc ;
      }

      sdbbson_destroy( &recordObj ) ;
   }

   return rc ;
}



/* connection pool */
SdbConnectionPool *sdbGetConnectionPool()
{
   static SdbConnectionPool connPool ;
   return &connPool ;
}

IndexPath *sdb_create_index_path(PlannerInfo *root,
              RelOptInfo *rel,
              IndexOptInfo *index,
              List *indexclauses,
              List *indexclausecols,
              List *indexorderbys,
              List *indexorderbycols,
              List *pathkeys,
              ScanDirection indexscandir,
              bool indexonly,
              Relids required_outer,
              double loop_count,
              SdbExecState *fdw_state)
{
   IndexPath  *pathnode = makeNode(IndexPath);
   List *indexquals ;
   List *indexqualcols;
   INT32 rowWidth           = 0 ;
   FLOAT64 selectivity      = 0.0 ;
   FLOAT64 inputRowCount    = 0.0 ;
   FLOAT64 foreignTableSize = 0.0 ;
   FLOAT64 totalCPUCost     = 0.0 ;
   BlockNumber pageCount    = 0 ;
   FLOAT64 totalDiskCost    = 0.0 ;

   pathnode->path.pathtype = indexonly ? T_IndexOnlyScan : T_IndexScan;
   pathnode->path.parent = rel;
   pathnode->path.param_info = get_baserel_parampathinfo(root, rel,
                                            required_outer);
   pathnode->path.pathkeys = pathkeys;

   /* Convert clauses to indexquals the executor can handle */
   sdb_expand_indexqual_conditions(indexclauses, indexclausecols,
                        &indexquals, &indexqualcols);

   /* Fill in the pathnode */
   pathnode->indexinfo = index;
   pathnode->indexclauses = indexclauses;
   pathnode->indexquals = indexquals;
   pathnode->indexqualcols = indexqualcols;
   pathnode->indexorderbys = indexorderbys;
   pathnode->indexorderbycols = indexorderbycols;
   pathnode->indexscandir = indexscandir;
#ifdef SDB_USE_OWN_POSTGRES
   pathnode->fdw_private = serializeSdbExecState( fdw_state ) ;
#endif /* SDB_USE_OWN_POSTGRES */

   /* calculate cost */
   selectivity      = clauselist_selectivity( root, rel->baserestrictinfo,
                                               0, JOIN_INNER, NULL ) ;
   inputRowCount    = 1 ;
   pathnode->path.rows = 1;
   pathnode->path.startup_cost = rel->baserestrictcost.startup;
   totalCPUCost = cpu_tuple_cost * pathnode->path.rows +
                  ( SDB_TUPLE_COST_MULTIPLIER * cpu_index_tuple_cost +
                   rel->baserestrictcost.per_tuple ) * inputRowCount ;

   rowWidth     = get_relation_data_width( fdw_state->tableID,
                                           rel->attr_widths ) ;
   foreignTableSize = rowWidth * pathnode->path.rows ;
   pageCount =( BlockNumber )rint( foreignTableSize / BLCKSZ ) ;
   totalDiskCost = seq_page_cost * pageCount ;


   pathnode->path.startup_cost = 0.42749999999999999 ;
   pathnode->path.total_cost = 7.676907287504247 ;

   return pathnode;
}

void sdb_expand_indexqual_conditions(List *indexclauses,
                     List *indexclausecols, List **indexquals_p,
                     List **indexqualcols_p)
{
   List     *indexquals = NIL;
   List     *indexqualcols = NIL;
   ListCell   *lcc,
            *lci;

   forboth(lcc, indexclauses, lci, indexclausecols)
   {
      RestrictInfo *rinfo = (RestrictInfo *) lfirst(lcc);
      int         indexcol = lfirst_int(lci);
      Expr     *clause = rinfo->clause;

      /* First check for boolean cases */

      /*
       * Else it must be an opclause (usual case), ScalarArrayOp,
       * RowCompare, or NullTest
       */
      if ( clause!= NULL && IsA(clause, OpExpr) )
      {
         indexquals = list_concat(indexquals, list_make1(rinfo));
         indexqualcols = lappend_int(indexqualcols, indexcol);
      }
   }

   *indexquals_p = indexquals;
   *indexqualcols_p = indexqualcols;
}

IndexPath *sdb_build_index_paths(PlannerInfo *root, RelOptInfo *rel,
              sdbIndexInfo *sdbIndex, sdbIndexClauseSet *clauses,
              SdbExecState *fdw_state)
{
   IndexPath  *ipath;
   List     *index_clauses;
   List     *clause_columns;
   Relids      outer_relids;
   double      loop_count;
   List     *orderbyclauses;
   List     *orderbyclausecols;
   List     *useful_pathkeys;
   bool     index_only_scan;
   int         indexcol;

   index_clauses = NIL;
   clause_columns = NIL;
   outer_relids = bms_copy(rel->lateral_relids);
   for (indexcol = 0; indexcol < sdbIndex->keyNum; indexcol++)
   {
      ListCell   *lc;

      foreach(lc, clauses->indexclauses[indexcol])
      {
         RestrictInfo *rinfo = (RestrictInfo *) lfirst(lc);
         index_clauses = lappend(index_clauses, rinfo);
         clause_columns = lappend_int(clause_columns, indexcol);
         outer_relids = bms_add_members(outer_relids,
                                 rinfo->clause_relids);
      }
   }

   /* We do not want the index's rel itself listed in outer_relids */
   outer_relids = bms_del_member(outer_relids, rel->relid);
   /* Enforce convention that outer_relids is exactly NULL if empty */
   if (bms_is_empty(outer_relids))
      outer_relids = NULL;

   /* Compute loop_count for cost estimation purposes */
   loop_count = sdb_get_loop_count(root, outer_relids);

   useful_pathkeys = NIL;
   orderbyclauses = NIL;
   orderbyclausecols = NIL;

   /*
    * 3. Check if an index-only scan is possible.  If we're not building
    * plain indexscans, this isn't relevant since bitmap scans don't support
    * index data retrieval anyway.
    */
   index_only_scan = false;

   ipath = sdb_create_index_path(root, rel, NULL,
                          index_clauses,
                          clause_columns,
                          orderbyclauses,
                          orderbyclausecols,
                          useful_pathkeys,
                          NoMovementScanDirection,
                          index_only_scan,
                          outer_relids,
                          loop_count,
                          fdw_state);
   return ipath;
}

double sdb_get_loop_count(PlannerInfo *root, Relids outer_relids)
{
   double      result = 1.0;

   /* For a non-parameterized path, just return 1.0 quickly */
   if (outer_relids != NULL)
   {
      int         relid;

      /* Need a working copy since bms_first_member is destructive */
      outer_relids = bms_copy(outer_relids);
      while ((relid = bms_first_member(outer_relids)) >= 0)
      {
         RelOptInfo *outer_rel;

         /* Paranoia: ignore bogus relid indexes */
         if (relid >= root->simple_rel_array_size)
            continue;
         outer_rel = root->simple_rel_array[relid];
         if (outer_rel == NULL)
            continue;
         Assert(outer_rel->relid == relid);  /* sanity check on array */

         /* Other relation could be proven empty, if so ignore */
         if (IS_DUMMY_REL(outer_rel))
            continue;

         /* Otherwise, rel's rows estimate should be valid by now */
         Assert(outer_rel->rows > 0);

         /* Remember smallest row count estimate among the outer rels */
         if (result == 1.0 || result > outer_rel->rows)
            result = outer_rel->rows;
      }
      bms_free(outer_relids);
   }
   return result;
}

EnumSdbArgType getArgumentType(List *arguments)
{
   INT32 varCount         = 0 ;
   INT32 ConstCount       = 0 ;
   INT32 ParamCount       = 0 ;
   ListCell *argumentCell = NULL ;

   if ( 2 != list_length(arguments) )
   {
      return SDB_UNSUPPORT_ARG_TYPE ;
   }

   foreach( argumentCell, arguments )
   {
      Expr *argument = (Expr *)lfirst(argumentCell) ;
      NodeTag argType = nodeTag(argument) ;
      if( argType == T_Var )
      {
         varCount++ ;
      }
      else if ( argType == T_Param )
      {
         ParamCount++ ;
      }
      else if ( argType == T_Const )
      {
         ConstCount++ ;
      }
      else if ( argType == T_RelabelType )
      {
         RelabelType *relabel = (RelabelType *)argument ;
         NodeTag relabelArgType = nodeTag(relabel->arg) ;
         if ( relabelArgType == T_Var )
         {
            varCount++ ;
         }
         else if ( relabelArgType == T_Param )
         {
            ParamCount++ ;
         }
         else if ( relabelArgType == T_Const )
         {
            ConstCount++ ;
         }
         else
         {
            elog( WARNING, "unsupport argument type:type=%d", relabelArgType ) ;
            return SDB_UNSUPPORT_ARG_TYPE ;
         }
      }
      else
      {
         elog( WARNING, "unsupport argument type:type=%d", argType ) ;
         return SDB_UNSUPPORT_ARG_TYPE ;
      }
   }

   if ( 2 == varCount )
   {
      return SDB_VAR_VAR ;
   }
   else if ( 1 ==  varCount && 1 == ConstCount )
   {
      return SDB_VAR_CONST ;
   }
   else if ( 1 == varCount && 1 == ParamCount )
   {
      return SDB_PARAM_VAR ;
   }

   return SDB_UNSUPPORT_ARG_TYPE ;
}

int sdbGenerateRescanCondition(SdbExecState *fdw_state, PlanState *planState,
                               sdbbson *rescanCondition)
{
   ListCell *cell     = NULL ;
   int rc             = SDB_OK ;
   SdbExprTreeState expr_state ;

   if ( bms_is_empty(planState->chgParam) )
   {
      sdbbson_finish( rescanCondition ) ;
      goto error ;
   }

   memset(&expr_state, 0, sizeof(SdbExprTreeState)) ;
   expr_state.foreign_table_index = fdw_state->relid ;
   expr_state.foreign_table_id    = fdw_state->tableID ;
   expr_state.is_use_decimal      = fdw_state->isUseDecimal ;

   sdbbson_init( rescanCondition ) ;
   foreach( cell, planState->qual )
   {
      INT32 rcTmp = SDB_OK ;
      sdbbson subBson ;
      RestrictInfo *info =(RestrictInfo *)lfirst(cell) ;
      sdbbson_init( &subBson ) ;
      rcTmp = sdbRecurExprTree( (Node *)info->clause, &expr_state,
                                &subBson, planState->ps_ExprContext ) ;
      sdbbson_finish( &subBson ) ;
      if( SDB_OK == rcTmp )
      {
         if ( !sdbbson_is_empty( &subBson ) )
         {
            sdbPrintBson( &subBson, DEBUG1, "rescan_sub" ) ;
            sdbbson_append_elements( rescanCondition, &subBson ) ;
         }
      }

      sdbbson_destroy( &subBson ) ;
   }

   rc = sdbbson_finish( rescanCondition ) ;
   if ( SDB_OK != rc )
   {
      sdbbson_destroy( rescanCondition ) ;
      sdbbson_init( rescanCondition ) ;
      sdbbson_finish( rescanCondition ) ;
   }

done:
   return rc ;
error:
   rc = -1 ;
   goto done ;
}

void sdbPrintBson( sdbbson *bson, int log_level, const char *label )
{
   int bufferSize = 0 ;
   char *p        = NULL ;
   char *myLabel  = (char *)label ;

   if ( NULL == label || label[0] == '\0' )
   {
      myLabel = "";
   }

   bufferSize = sdbbson_sprint_length( bson ) ;
   p = ( char* )malloc( bufferSize ) ;
   sdbbson_sprint( p, bufferSize, bson ) ;

   ereport( log_level, ( errcode( ERRCODE_FDW_ERROR ),
            errmsg( "bson value=%s,label[%s]", p, myLabel ) ) ) ;

   free( p ) ;
}

void SdbInitRecordCache()
{
   INT32 i = 0 ;
   SdbRecordCache *cache = SdbGetRecordCache() ;
   cache->size      = SDB_MAX_RECORD_SIZE ;
   cache->usedCount = 0 ;
   for (; i < cache->size; i++ )
   {
      cache->recordArray[i].record = malloc( sizeof( sdbbson ) ) ;
      sdbbson_init( cache->recordArray[i].record ) ;
      cache->recordArray[i].isUsed = FALSE ;
   }
}

void SdbFiniRecordCache()
{
   INT32 i = 0 ;
   SdbRecordCache *cache = SdbGetRecordCache() ;

   for (; i < cache->size; i++ )
   {
      sdbbson_destroy( cache->recordArray[i].record ) ;
      free( cache->recordArray[i].record ) ;
      cache->recordArray[i].record = NULL ;
   }

   cache->usedCount = 0 ;
   cache->size      = 0 ;
}

SdbRecordCache *SdbGetRecordCache()
{
   static SdbRecordCache cache ;
   return &cache ;
}

sdbbson *SdbAllocRecord( SdbRecordCache *recordCache, UINT64 *recordID )
{
   INT32 i = 0 ;
   sdbbson *pRecord ;

   for ( ; i < recordCache->size ; i++ )
   {
      if ( !recordCache->recordArray[i].isUsed )
      {
         pRecord = recordCache->recordArray[i].record ;
         recordCache->recordArray[i].isUsed = TRUE ;
         *recordID = i ;

         recordCache->usedCount++ ;
         elog( DEBUG1, "SdbAllocRecord:usedCount=%d,index=%d",
               recordCache->usedCount, i ) ;
         return pRecord ;
      }
   }

   elog( ERROR, "SdbAllocRecord failed:usedCount=%d,index=%d",
         recordCache->usedCount, i) ;
   return NULL ;
}

sdbbson *SdbGetRecord( SdbRecordCache *recordCache, UINT64 recordID )
{
   INT32 index = ( INT32 ) recordID ;

   if ( index >= recordCache->size )
   {
      elog( ERROR, "recordID is not correct:recordID=%d", index ) ;
      return NULL ;
   }

   if ( !recordCache->recordArray[index].isUsed )
   {
      elog( DEBUG1, "get released record!!!:index=%d", index ) ;
      recordCache->recordArray[index].isUsed = TRUE ;
      recordCache->usedCount++ ;
   }

   return recordCache->recordArray[index].record ;
}

void SdbReleaseRecord( SdbRecordCache *recordCache, UINT64 recordID )
{
   INT32 index = ( INT32 ) recordID ;

   if ( index >= recordCache->size || index < 0 )
   {
      elog( ERROR, "recordID is not correct:recordID=%d", index ) ;
      return ;
   }

   if ( !recordCache->recordArray[index].isUsed )
   {
      return ;
   }

   recordCache->recordArray[index].isUsed = FALSE ;
   recordCache->usedCount-- ;
}


