#include "postgres.h"
#include "sdb_fdw.h"
#include "msgDef.h"
#include "msg.h"
#include "sdb_fdw_util.h"

#include "time.h"

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

#include <pthread.h>

#define SDB_COLLECTIONSPACE_NAME_LEN 128
#define SDB_COLLECTION_NAME_LEN      128
#define SDB_COLUMNA_ID_NAME          "_id"
#define SDB_SHARDINGKEY_NAME         "ShardingKey"

#define SDB_MSECS_TO_USECS           ( 1000L )
#define SDB_SECONDS_TO_USECS         ( 1000L * 1000L )
#define SDB_MSECS_PER_HOUR           ( USECS_PER_HOUR/1000 )


#define SDB_FIELD_COMMA              ","
#define SDB_FIELD_SEMICOLON          ":"
#define SDB_FIELD_SEMICOLON_CHR      ':'

#define SDB_KEYNAME_REGEX            "$regex"

#define SDB_FIELD_VALUE_LEN          (128)

static bool SdbIsListMode( CHAR *hostName ) ;

static int SdbSetNodeAddressInfo( SdbInputOptions *options, CHAR *hostName,
                                  CHAR *service ) ;

static void SdbGetForeignRelSize( PlannerInfo *root,
                                   RelOptInfo *baserel,
                                   Oid foreignTableId ) ;

static void SdbGetForeignPaths( PlannerInfo *root,
                                 RelOptInfo *baserel,
                                 Oid foreignTableId ) ;

static ForeignScan *SdbGetForeignPlan( PlannerInfo *root,
                                        RelOptInfo *baserel,
                                        Oid foreignTableId,
                                        ForeignPath *bestPath,
                                        List *targetList,
                                        List *restrictionClauses ) ;

static void SdbBeginForeignScan( ForeignScanState *scanState,
                                  INT32 executorFlags ) ;

static TupleTableSlot *SdbIterateForeignScan( ForeignScanState *scanState ) ;

static void SdbRescanForeignScan( ForeignScanState *scanState ) ;

static void SdbEndForeignScan( ForeignScanState *scanState ) ;

static void SdbExplainForeignScan( ForeignScanState *scanState,
                                    ExplainState *explainState ) ;

static bool SdbAnalyzeForeignTable( Relation relation,
                                     AcquireSampleRowsFunc *acquireSampleRowsFunc,
                                     BlockNumber *totalPageCount ) ;

static INT32 SdbIsForeignRelUpdatable( Relation rel ) ;

static void SdbAddForeignUpdateTargets( Query *parsetree,
                                         RangeTblEntry *target_rte,
                                         Relation target_relation ) ;

static List *SdbPlanForeignModify ( PlannerInfo *root,
                                    ModifyTable *plan,
                                    Index resultRelation,
                                    INT32 subplan_index ) ;

static void sdb_slot_deform_tuple( TupleTableSlot *slot, int natts ) ;

static Var *getRealVar(Var *arg);


typedef struct
{
   pthread_t checkThreadID ;
   int running ;
} sdbCheckThreadInfo;


static sdbCheckThreadInfo *sdbGetCheckThreadInfo() ;

static void sdbStartInterruptCheck() ;

static void sdbJoinInterruptThread() ;


static void *thr_check_interrupt( void * arg ) ;

static void sdbInterruptAllConnection() ;


/* module initialization */
void _PG_init (  ) ;

/* transaction management */
static void SdbFdwXactCallback( XactEvent event, void *arg ) ;

/* sequoiadb's statistic info */
static void SdbDestroyCLStatistics( SdbCLStatistics *clStat, bool freeObj );
static INT32 SdbInitCLStatistics( SdbCLStatistics *clStat );
static void SdbFiniCLStatisticsCache( SdbStatisticsCache *cache );

static BOOLEAN isNormalChar( CHAR c ) ;
static CHAR *changeToRegexFormat( CHAR *outputString ) ;


#if PG_VERSION_NUM>90300
static void SdbBeginForeignModify( ModifyTableState *mtstate,
      ResultRelInfo *rinfo, List *fdw_private, int subplan_index, int eflags ) ;

static TupleTableSlot *SdbExecForeignInsert( EState *estate, ResultRelInfo *rinfo,
      TupleTableSlot *slot, TupleTableSlot *planSlot ) ;

static TupleTableSlot *SdbExecForeignDelete( EState *estate, ResultRelInfo *rinfo,
      TupleTableSlot *slot, TupleTableSlot *planSlot ) ;

static TupleTableSlot *SdbExecForeignUpdate( EState *estate, ResultRelInfo *rinfo,\
      TupleTableSlot *slot, TupleTableSlot *planSlot ) ;

static void SdbEndForeignModify( EState *estate, ResultRelInfo *rinfo ) ;

static void SdbExplainForeignModify( ModifyTableState *mtstate, ResultRelInfo *rinfo,
      List *fdw_private, int subplan_index, struct ExplainState *es ) ;

#endif

/* register handler and validator */
PG_MODULE_MAGIC ;
PG_FUNCTION_INFO_V1( sdb_fdw_handler ) ;
PG_FUNCTION_INFO_V1( sdb_fdw_validator ) ;

/* declare for sdb_fdw_handler */
Datum sdb_fdw_handler( PG_FUNCTION_ARGS )
{
   FdwRoutine *fdwRoutine              = makeNode( FdwRoutine ) ;
   /* Functions for scanning foreign tables */
   fdwRoutine->GetForeignRelSize       = SdbGetForeignRelSize ;
   fdwRoutine->GetForeignPaths         = SdbGetForeignPaths ;
   fdwRoutine->GetForeignPlan          = SdbGetForeignPlan ;
   fdwRoutine->BeginForeignScan        = SdbBeginForeignScan ;
   fdwRoutine->IterateForeignScan      = SdbIterateForeignScan ;
   fdwRoutine->ReScanForeignScan       = SdbRescanForeignScan ;
   fdwRoutine->EndForeignScan          = SdbEndForeignScan ;
#if PG_VERSION_NUM>90300
   /* Remaining functions are optional */
   fdwRoutine->AddForeignUpdateTargets = SdbAddForeignUpdateTargets ;
   fdwRoutine->PlanForeignModify       = SdbPlanForeignModify ;
   fdwRoutine->BeginForeignModify      = SdbBeginForeignModify ;
   fdwRoutine->ExecForeignInsert       = SdbExecForeignInsert ;
   fdwRoutine->ExecForeignUpdate       = SdbExecForeignUpdate ;
   fdwRoutine->ExecForeignDelete       = SdbExecForeignDelete ;
   fdwRoutine->EndForeignModify        = SdbEndForeignModify ;
   fdwRoutine->IsForeignRelUpdatable   = SdbIsForeignRelUpdatable ;
#endif
   /* Support functions for EXPLAIN */
   fdwRoutine->ExplainForeignScan      = SdbExplainForeignScan ;
#if PG_VERSION_NUM>90300
   fdwRoutine->ExplainForeignModify    = SdbExplainForeignModify ;
#endif
   fdwRoutine->AnalyzeForeignTable     = SdbAnalyzeForeignTable ;
   PG_RETURN_POINTER( fdwRoutine ) ;
}

static void sdbGetOptions( Oid foreignTableId, SdbInputOptions *options ) ;
static int sdbGetSdbServerOptions( Oid foreignTableId, SdbExecState *sdbExecState ) ;

static PgTableDesc *sdbGetPgTableDesc( Oid foreignTableId ) ;
static void initSdbExecState( SdbExecState *sdbExecState ) ;
static void sdbFreeScanState( SdbExecState *executionState, bool deleteShared ) ;
static void sdbFreeScanStateInModifyEnd( SdbExecState *executionState ) ;

static Const *sdbSerializeDocument( sdbbson *document ) ;
static void sdbDeserializeDocument( Const *constant, sdbbson *document ) ;

#define serializeInt( x )makeConst( INT4OID, -1, InvalidOid, 4, Int32GetDatum( ( int32 )( x ) ), 0, 1 )
#define serializeOid( x )makeConst( OIDOID, -1, InvalidOid, 4, ObjectIdGetDatum( x ), 0, 1 )
static Const *serializeString( const char *s ) ;
static Const *serializeUint64( UINT64 value ) ;

static char *deserializeString( Const *constant ) ;
static UINT64 deserializeUint64( Const *constant ) ;
static Oid deserializeOid( Const *constant ) ;
static SdbExecState *deserializeSdbExecState( List *sdbExecStateList ) ;

static INT32 sdbAppendConstantValue( sdbbson *bsonObj, const char *keyName,
                                     Const *constant, INT32 isUseDecimal ) ;


static UINT64 sdbCreateBsonRecordAddr(  ) ;
static sdbbson* sdbGetRecordPointer( UINT64 record_addr ) ;

static void sdbGetColumnKeyInfo( SdbExecState *fdw_state ) ;

static bool sdbIsShardingKeyChanged( SdbExecState *fdw_state, sdbbson *oldBson,
      sdbbson *newBson ) ;

static UINT64 sdbbson_iterator_getusecs( sdbbson_iterator *ite ) ;

static INT32 sdbOperExprParamVar( OpExpr *expr, SdbExprTreeState *expr_state,
                           sdbbson *condition, ExprContext *exprContext ) ;
static INT32 sdbOperExprTwoVar( OpExpr *opr_two_argument,
                                SdbExprTreeState *expr_state,
                                sdbbson *condition ) ;
static INT32 sdbOperExpr( OpExpr *opr, SdbExprTreeState *expr_state,
                          sdbbson *condition, ExprContext *exprContext ) ;

static INT32 sdbScalarArrayOpExpr( ScalarArrayOpExpr *scalaExpr,
                                 SdbExprTreeState *expr_state,
                                 sdbbson *condition ) ;

static INT32 sdbVarExpr( Var *var, SdbExprTreeState *expr_state,
                  sdbbson *condition ) ;

static INT32 sdbNullTestExpr( NullTest *ntest, SdbExprTreeState *expr_state,
                              sdbbson *condition ) ;
static INT32 sdbRecurBoolExpr( BoolExpr *boolexpr, SdbExprTreeState *expr_state,
                               sdbbson *condition, ExprContext *exprContext ) ;

static INT32 sdbBooleanTestIsNotExpr( const char *columnName, sdbbson *condition,
                                      BOOLEAN value ) ;

static INT32 sdbBooleanTestExpr( BooleanTest *boolTest,
                                 SdbExprTreeState *expr_state,
                                 sdbbson *condition ) ;

static INT32 sdbGenerateFilterCondition( Oid foreign_id, RelOptInfo *baserel,
                                         INT32 isUseDecimal, sdbbson *condition ) ;

static const CHAR *sdbOperatorName( const CHAR *operatorName,
                                    BOOLEAN isVarFirst ) ;

static Expr *sdbFindArgumentOfType( List *argumentList, NodeTag argumentType,
                                    INT32 *index ) ;


bool SdbIsListMode( CHAR *hostName )
{
   INT32 i      = 0 ;
   INT32 len    = strlen( hostName ) ;
   for( i = 0 ; i < len ; i++ )
   {
      if ( hostName[i] == SDB_FIELD_SEMICOLON_CHR )
      {
         return true ;
      }
   }

   return false ;
}

int SdbSetNodeAddressInfo( SdbInputOptions *options, CHAR *hostName,
                           CHAR *service )
{
   CHAR tmpName[ SDB_MAX_SERVICE_LENGTH + 1 ] ;
   snprintf( tmpName, SDB_MAX_SERVICE_LENGTH, "%s", hostName ) ;
   ereport( DEBUG1, ( errcode( ERRCODE_FDW_ERROR ),
            errmsg( "hostName=%s,service=%s", tmpName, service ) ) ) ;
   if( !SdbIsListMode( tmpName ) )
   {
      StringInfo result = makeStringInfo() ;
      appendStringInfo( result, "%s%s%s", tmpName, SDB_FIELD_SEMICOLON,
                        service ) ;
      options->serviceNum     = 1 ;
      options->serviceList[0] = pstrdup( result->data ) ;
   }
   else
   {
      CHAR *tmpSrc    = tmpName ;
      CHAR *tmpPtr    = NULL ;
      CHAR *tmpResult = NULL ;
      INT32 i = 0 ;
      while( ( tmpResult = strtok_r( tmpSrc, SDB_FIELD_COMMA, &tmpPtr ) )
             != NULL )
      {
         tmpSrc = NULL ;
         if ( i >= INITIAL_ARRAY_CAPACITY )
         {
            break ;
         }

         options->serviceList[i] = pstrdup( tmpResult ) ;
         i++ ;
      }

      options->serviceNum = i ;
   }

   return SDB_OK ;
}



int sdbGetSdbServerOptions( Oid foreignTableId, SdbExecState *sdbExecState )
{
   INT32 i = 0 ;
   SdbInputOptions options ;
   sdbGetOptions( foreignTableId, &options ) ;
   for ( i = 0 ; i < options.serviceNum; i++ )
   {
      sdbExecState->sdbServerList[i] = options.serviceList[i] ;
   }
   sdbExecState->sdbServerNum  = options.serviceNum;
   sdbExecState->sdbcs         = options.collectionspace ;
   sdbExecState->sdbcl         = options.collection ;
   sdbExecState->usr           = options.user ;
   sdbExecState->passwd        = options.password ;
   sdbExecState->preferenceInstance = options.preference_instance ;
   sdbExecState->transaction   = options.transaction ;
   sdbExecState->isUseDecimal  = options.isUseDecimal ;

   return 0 ;
}

PgTableDesc *sdbGetPgTableDesc( Oid foreignTableId )
{
   int i = 0 ;
   Relation rel      = heap_open( foreignTableId, NoLock ) ;
   TupleDesc tupdesc = rel->rd_att ;

   PgTableDesc *tableDesc = palloc( sizeof( PgTableDesc ) ) ;
   tableDesc->ncols = tupdesc->natts ;
   tableDesc->name  = get_rel_name( foreignTableId ) ;
   tableDesc->cols  = palloc( tableDesc->ncols * sizeof( PgColumnsDesc ) ) ;

   for ( i = 0 ; i < tupdesc->natts ; i++ )
   {
      Form_pg_attribute att_tuple = tupdesc->attrs[i] ;
      tableDesc->cols[i].isDropped = false ;
      tableDesc->cols[i].pgattnum  = att_tuple->attnum ;
      tableDesc->cols[i].pgtype    = att_tuple->atttypid ;
      tableDesc->cols[i].pgtypmod  = att_tuple->atttypmod ;
      tableDesc->cols[i].pgname    = pstrdup( NameStr( att_tuple->attname ) ) ;

      if ( att_tuple->attisdropped )
      {
         tableDesc->cols[i].isDropped = true ;
      }
   }

   heap_close( rel, NoLock ) ;
   return tableDesc ;
}

void initSdbExecState( SdbExecState *sdbExecState )
{
   memset( sdbExecState, 0, sizeof( SdbExecState ) ) ;
   sdbExecState->hCursor           = SDB_INVALID_HANDLE ;
   sdbExecState->hConnection       = SDB_INVALID_HANDLE ;
   sdbExecState->hCollection       = SDB_INVALID_HANDLE ;

   sdbExecState->bson_record_addr  = -1 ;

   sdbbson_init( &sdbExecState->queryDocument ) ;
   sdbbson_finish( &sdbExecState->queryDocument ) ;
}

Const *serializeString( const char *s )
{
   if ( s == NULL )
      return makeNullConst( TEXTOID, -1, InvalidOid ) ;
   else
      return makeConst( TEXTOID, -1, InvalidOid, -1, PointerGetDatum( cstring_to_text( s ) ), 0, 0 ) ;
}

Const *serializeUint64( UINT64 value )
{
   return makeConst( INT4OID, -1, InvalidOid, 8, Int64GetDatum( ( int64 )value ),
      #ifdef USE_FLOAT8_BYVAL
          1,
      #else
          0,
      #endif  /* USE_FLOAT8_BYVAL */
          0 ) ;
}


List *serializeSdbExecState( SdbExecState *fdwState )
{
   List *result = NIL ;
   int i        = 0 ;

   /* sdbServerNum */
   result = lappend( result, serializeInt( fdwState->sdbServerNum ) ) ;

   /* sdbServerList */
   for ( i = 0 ; i < fdwState->sdbServerNum ; i++ )
   {
      result = lappend( result, serializeString( fdwState->sdbServerList[i]) ) ;
   }

   /* usr */
   result = lappend( result, serializeString( fdwState->usr ) ) ;
   /* passwd */
   result = lappend( result, serializeString( fdwState->passwd ) ) ;
   /* preferenceInstance */
   result = lappend( result, serializeString( fdwState->preferenceInstance ) ) ;
   /* transaction */
   result = lappend( result, serializeString( fdwState->transaction ) ) ;

   /* sdbcs */
   result = lappend( result, serializeString( fdwState->sdbcs ) ) ;
   /* sdbcl */
   result = lappend( result, serializeString( fdwState->sdbcl ) ) ;

   /* table name */
   result = lappend( result, serializeString( fdwState->pgTableDesc->name ) ) ;
   /* number of columns in the table */
   result = lappend( result, serializeInt( fdwState->pgTableDesc->ncols ) ) ;

   /* column data */
   for ( i = 0 ; i<fdwState->pgTableDesc->ncols ; ++i )
   {
      result = lappend( result, serializeString( fdwState->pgTableDesc->cols[i].pgname ) ) ;
      result = lappend( result, serializeInt( fdwState->pgTableDesc->cols[i].pgattnum ) ) ;
      result = lappend( result, serializeOid( fdwState->pgTableDesc->cols[i].pgtype ) ) ;
      result = lappend( result, serializeInt( fdwState->pgTableDesc->cols[i].pgtypmod ) ) ;
   }

   /* queryDocument */
   result = lappend( result, sdbSerializeDocument( &fdwState->queryDocument ) ) ;

   /* _id_addr */
   result = lappend( result, serializeUint64( fdwState->bson_record_addr ) ) ;

   /* isUseDecimal */
   result = lappend( result, serializeInt( fdwState->isUseDecimal ) ) ;

   /* tableID */
   result = lappend( result, serializeOid( fdwState->tableID ) ) ;

   /* relid*/
   result = lappend( result, serializeInt( fdwState->relid) ) ;

   /* key_num */
   result = lappend( result, serializeInt( fdwState->key_num ) ) ;
   for ( i = 0 ; i < fdwState->key_num ; i++ )
   {
      result = lappend( result, serializeString( fdwState->key_name[i] ) ) ;
   }

   return result ;
}

SdbExecState *deserializeSdbExecState( List *sdbExecStateList )
{
   ListCell *cell = NULL ;
   int i = 0 ;
   SdbExecState *fdwState = ( SdbExecState* )palloc( sizeof( SdbExecState ) ) ;
   initSdbExecState( fdwState ) ;

   cell = list_head( sdbExecStateList ) ;

   /* sdbServerNum */
   fdwState->sdbServerNum = ( int )DatumGetInt32( ( ( Const * )lfirst( cell ) )->constvalue ) ;
   cell = lnext( cell ) ;

   /* sdbServerList */
   for ( i = 0 ; i < fdwState->sdbServerNum ; ++i )
   {
      fdwState->sdbServerList[i] = deserializeString( lfirst( cell ) ) ;
      cell = lnext( cell ) ;
   }

   /* usr */
   fdwState->usr = deserializeString( lfirst( cell ) ) ;
   cell = lnext( cell ) ;

   /* passwd */
   fdwState->passwd = deserializeString( lfirst( cell ) ) ;
   cell = lnext( cell ) ;

   /* preferenceInstance */
   fdwState->preferenceInstance = deserializeString( lfirst( cell ) ) ;
   cell = lnext( cell ) ;

   /* preferenceInstance */
   fdwState->transaction = deserializeString( lfirst( cell ) ) ;
   cell = lnext( cell ) ;

   /* sdbcs */
   fdwState->sdbcs = deserializeString( lfirst( cell ) ) ;
   cell = lnext( cell ) ;
   /* sdbcl */
   fdwState->sdbcl = deserializeString( lfirst( cell ) ) ;
   cell = lnext( cell ) ;

   /* table name */
   fdwState->pgTableDesc = palloc( sizeof( PgTableDesc ) ) ;
   fdwState->pgTableDesc->name = deserializeString( lfirst( cell ) ) ;
   cell = lnext( cell ) ;

   /* number of columns in the table */
   fdwState->pgTableDesc->ncols= ( int )DatumGetInt32( ( ( Const * )lfirst( cell ) )->constvalue ) ;
   cell = lnext( cell ) ;

   fdwState->pgTableDesc->cols = palloc( fdwState->pgTableDesc->ncols * sizeof( PgColumnsDesc ) ) ;
   /* column data */
   for ( i = 0 ; i < fdwState->pgTableDesc->ncols ; ++i )
   {
      fdwState->pgTableDesc->cols[i].pgname = deserializeString( lfirst( cell ) ) ;
      cell = lnext( cell ) ;
      fdwState->pgTableDesc->cols[i].pgattnum = ( int )DatumGetInt32( ( ( Const * )lfirst( cell ) )->constvalue ) ;
      cell = lnext( cell ) ;
      fdwState->pgTableDesc->cols[i].pgtype = DatumGetObjectId( ( ( Const * )lfirst( cell ) )->constvalue ) ;
      cell = lnext( cell ) ;
      fdwState->pgTableDesc->cols[i].pgtypmod = ( int )DatumGetInt32( ( ( Const * )lfirst( cell ) )->constvalue ) ;
      cell = lnext( cell ) ;
   }

   sdbbson_destroy( &fdwState->queryDocument ) ;
   sdbDeserializeDocument( ( Const * )lfirst( cell ), &fdwState->queryDocument ) ;
   cell = lnext( cell ) ;

   /* _id_addr */
   fdwState->bson_record_addr = deserializeUint64( lfirst( cell ) ) ;
   cell = lnext( cell ) ;

   /* isUseDecimal */
   fdwState->isUseDecimal = ( int )DatumGetInt32( ( ( Const * )lfirst( cell ) )->constvalue ) ;
   cell = lnext( cell ) ;

   /* tableID */
   fdwState->tableID = deserializeOid( lfirst( cell ) ) ;
   cell = lnext( cell ) ;

   /* relid */
   fdwState->relid = (Index)DatumGetInt32(((Const *)lfirst(cell))->constvalue) ;
   cell = lnext( cell ) ;

   /* key_num */
   fdwState->key_num = ( int )DatumGetInt32( ( ( Const * )lfirst( cell ) )->constvalue ) ;
   cell = lnext( cell ) ;

   for ( i = 0 ; i < fdwState->key_num ; i++ )
   {
      strncpy( fdwState->key_name[i], deserializeString( lfirst( cell ) ),
            SDB_MAX_KEY_COLUMN_LENGTH - 1 ) ;
      cell = lnext( cell ) ;
   }

   return fdwState ;
}

char *deserializeString( Const *constant )
{
   if ( constant->constisnull )
      return NULL ;
   else
      return text_to_cstring( DatumGetTextP( constant->constvalue ) ) ;
}

UINT64 deserializeUint64( Const *constant )
{
   return ( UINT64 )DatumGetInt64( constant->constvalue ) ;
}

Oid deserializeOid( Const *constant )
{
   return ( Oid )DatumGetObjectId( constant->constvalue ) ;
}

/*long deserializeLong( Const *constant )
{
   if ( sizeof( long )<= 4 )
      return ( long )DatumGetInt32( constant->constvalue ) ;
   else
      return ( long )DatumGetInt64( constant->constvalue ) ;
}*/

int sdbSetBsonValue( sdbbson *bsonObj, const char *name, Datum valueDatum,
         Oid columnType, INT32 columnTypeMod, INT32 isUseDecimal )
{
   INT32 rc = SDB_OK ;
   switch( columnType )
   {
      case INT2OID :
      {
         INT16 value = DatumGetInt16( valueDatum ) ;
         sdbbson_append_int( bsonObj, name, ( INT32 )value ) ;
         break ;
      }

      case INT4OID :
      {
         INT32 value = DatumGetInt32( valueDatum ) ;
         sdbbson_append_int( bsonObj, name, value ) ;
         break ;
      }

      case INT8OID :
      {
         INT64 value = DatumGetInt64( valueDatum ) ;
         sdbbson_append_long( bsonObj, name, value ) ;
         break ;
      }

      case FLOAT4OID :
      {
         FLOAT32 value = DatumGetFloat4( valueDatum ) ;
         sdbbson_append_double( bsonObj, name, ( FLOAT64 )value ) ;
         break ;
      }

      case FLOAT8OID :
      {
         FLOAT64 value = DatumGetFloat8( valueDatum ) ;
         sdbbson_append_double( bsonObj, name, value ) ;
         break ;
      }

      case NUMERICOID :
      {
         char *value ;
         Datum formatted ;
         Datum NumericStr ;
         formatted  = DirectFunctionCall2( numeric, valueDatum, columnTypeMod ) ;
         NumericStr = DirectFunctionCall1( numeric_out, formatted ) ;
         value      = DatumGetCString( NumericStr ) ;
         if ( isUseDecimal )
         {
            if (columnTypeMod < (int32) (VARHDRSZ))
            {
               sdbbson_append_decimal3( bsonObj, name, value ) ;
            }
            else
            {
               int32 tmp_typmod = columnTypeMod - VARHDRSZ;
               int precision    = (tmp_typmod >> 16) & 0xffff;
               int scale        = tmp_typmod & 0xffff;
               sdbbson_append_decimal2( bsonObj, name, value, precision, scale ) ;
            }
         }
         else
         {
            sdbbson_append_string( bsonObj, name, value ) ;
         }

         break ;
      }

      case BOOLOID :
      {
         BOOLEAN value = DatumGetBool( valueDatum ) ;
         sdbbson_append_bool( bsonObj, name, value ) ;
         break ;
      }

      case BPCHAROID :
      case VARCHAROID :
      case TEXTOID :
      {
         CHAR *outputString    = NULL ;
         Oid outputFunctionId  = InvalidOid ;
         bool typeVarLength    = false ;
         getTypeOutputInfo( columnType, &outputFunctionId, &typeVarLength ) ;
         outputString = OidOutputFunctionCall( outputFunctionId, valueDatum ) ;
         if ( strcmp( name, SDB_KEYNAME_REGEX ) == 0 )
         {
            CHAR *regexFormat = changeToRegexFormat( outputString ) ;
            if ( NULL != regexFormat )
            {
               sdbbson_append_string( bsonObj, name, regexFormat ) ;
               pfree( regexFormat ) ;
            }
            else
            {
               rc = -1 ;
               pfree( outputString ) ;
               elog( WARNING, "value can't change to regex:value=%s",
                     outputString ) ;
               goto error ;
            }
         }
         else
         {
            sdbbson_append_string( bsonObj, name, outputString ) ;
         }

         pfree( outputString ) ;
         break ;
      }

      case NAMEOID :
      {
         sdbbson_oid_t sdbbsonObjectId ;
         CHAR *outputString    = NULL ;
         Oid outputFunctionId  = InvalidOid ;
         bool typeVarLength    = false ;
         getTypeOutputInfo( columnType, &outputFunctionId, &typeVarLength ) ;
         outputString = OidOutputFunctionCall( outputFunctionId, valueDatum ) ;

         memset( sdbbsonObjectId.bytes, 0, sizeof( sdbbsonObjectId.bytes ) ) ;
         sdbbson_oid_from_string( &sdbbsonObjectId, outputString ) ;
         sdbbson_append_oid( bsonObj, name, &sdbbsonObjectId ) ;
         pfree( outputString ) ;
         break ;
      }

      case DATEOID :
      {
         Datum valueDatum_tmp = DirectFunctionCall1( date_timestamptz, valueDatum ) ;
         Timestamp valueTimestamp = DatumGetTimestamp( valueDatum_tmp ) ;
         INT64 valueUsecs         = valueTimestamp + POSTGRES_TO_UNIX_EPOCH_USECS ;
         INT64 valueMilliSecs     = valueUsecs / SDB_MSECS_TO_USECS ;

         /* here store the UTC time */
         sdbbson_append_date( bsonObj, name, valueMilliSecs ) ;
         break ;
      }

      case TIMESTAMPOID :
      case TIMESTAMPTZOID :
      {
         Datum valueDatum_tmp = DirectFunctionCall1( timestamp_timestamptz, valueDatum ) ;
         Timestamp valueTimestamp = DatumGetTimestamp( valueDatum_tmp ) ;
         INT64 valueUsecs         = valueTimestamp + POSTGRES_TO_UNIX_EPOCH_USECS ;
         sdbbson_timestamp_t bson_time ;
         bson_time.t = valueUsecs/SDB_SECONDS_TO_USECS ;
         bson_time.i = valueUsecs%SDB_SECONDS_TO_USECS ;

         sdbbson_append_timestamp( bsonObj, name, &bson_time ) ;
         break ;
      }
      case BYTEAOID :
      {
			CHAR *buff = VARDATA_ANY( ( bytea * )DatumGetPointer( valueDatum ) ) ;
			INT32 len  = VARSIZE_ANY_EXHDR( ( bytea * )DatumGetPointer( valueDatum ) ) ;
			elog(DEBUG1, "len=%d", len) ;
         sdbbson_append_binary( bsonObj, name, BSON_BIN_BINARY, buff, len ) ;
         break ;
      }

      case TEXTARRAYOID:
      case INT4ARRAYOID:
      case FLOAT4ARRAYOID:
      case 1022:
      /* FLOAT8ARRAY is not support */
      case INT2ARRAYOID:
      /* this type do not have type name, so we must use the value(see more types in pg_type.h) */
      case 1115:
      case 1182:
      case 1014:
      case 1231:
      case 1016:
      case 1000:
      case 1001:
      {
         INT32 i = 0 ;
         Datum datumTmp ;
         bool isNull            = false ;
         ArrayType *arr         = DatumGetArrayTypeP( valueDatum ) ;
         ArrayIterator iterator = array_create_iterator( arr , 0 ) ;
         Oid element_type       = ARR_ELEMTYPE( arr ) ;

         sdbbson_append_start_array( bsonObj, name ) ;
         while ( array_iterate( iterator, &datumTmp, &isNull ) )
         {
            CHAR arrayIndex[SDB_MAX_KEY_COLUMN_LENGTH + 1] = "" ;
            sprintf( arrayIndex, "%d", i ) ;
            rc = sdbSetBsonValue( bsonObj, arrayIndex, datumTmp,
                                  element_type, 0, isUseDecimal ) ;
            if ( SDB_OK != rc )
            {
               break ;
            }
            i++ ;
         }
         sdbbson_append_finish_array( bsonObj ) ;

         array_free_iterator(iterator);

         break ;
      }

      default :
      {
         /* we do not support other data types */
         ereport ( WARNING, ( errcode( ERRCODE_FDW_INVALID_DATA_TYPE ),
            errmsg( "Cannot convert constant value to BSON" ),
            errhint( "Constant value data type: %u", columnType ) ) ) ;

         return -1 ;
      }
   }
done:
   return rc ;
error:
   goto done ;
}

UINT64 sdbCreateBsonRecordAddr(  )
{
   UINT64 id = 0 ;
   SdbRecordCache *cache = SdbGetRecordCache() ;
   SdbAllocRecord( cache, &id ) ;

   return id ;
}

sdbbson *sdbGetRecordPointer( UINT64 record_addr )
{
   SdbRecordCache *cache = SdbGetRecordCache() ;
   return SdbGetRecord( cache, record_addr ) ;
}

void sdbGetColumnKeyInfo( SdbExecState *fdw_state )
{
   const SdbCLStatistics *clStat =
                SdbGetCLStatFromCache( fdw_state->tableID );
   if ( NULL == clStat )
   {
      ereport( WARNING,( errcode( ERRCODE_FDW_ERROR ),
                       errmsg( "cannot get stat of cl from cache" ),
                       errhint( "cs name: %s, cl name:%s",
                                fdw_state->sdbcs, fdw_state->sdbcl ) ) ) ;
      return ;
   }

   fdw_state->key_num = clStat->keyNum ;
   memcpy( fdw_state->key_name, clStat->shardingKeys,
           sizeof( clStat->shardingKeys ) ) ;
   return ;
}

INT32 sdbGetShardingKeyInfo( const SdbInputOptions *options,
                             SdbCLStatistics *clStat )
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle connection ;
   sdbCursorHandle cursor ;
   sdbbson condition ;
   sdbbson selector ;
   sdbbson ShardingKey ;

   sdbbson_iterator ite ;
   sdbbson_type type ;

   StringInfo fullCollectionName = makeStringInfo(  ) ;
   appendStringInfoString( fullCollectionName, options->collectionspace ) ;
   appendStringInfoString( fullCollectionName, "." ) ;
   appendStringInfoString( fullCollectionName, options->collection ) ;

   sdbbson_init( &condition ) ;
   sdbbson_init( &selector ) ;
   sdbbson_init( &ShardingKey ) ;

   /* first add the _id to the key_name */
   clStat->keyNum = 1 ;
   strncpy( clStat->shardingKeys[0], SDB_COLUMNA_ID_NAME,
         SDB_MAX_KEY_COLUMN_LENGTH - 1 ) ;

   /* get the cs.cl's shardingkey */
   connection = sdbGetConnectionHandle( ( const char ** )( options->serviceList ),
                                        options->serviceNum,
                                        options->user,
                                        options->password,
                                        options->preference_instance,
                                        options->transaction ) ;

   sdbbson_append_string( &condition, "Name", fullCollectionName->data ) ;
   rc = sdbbson_finish( &condition ) ;
   if ( SDB_OK != rc )
   {
      ereport( ERROR, ( errcode( ERRCODE_FDW_ERROR ),
               errmsg( "sdbbson_finish failed:rc=%d", rc ) ) ) ;
      goto error ;
   }

   sdbbson_append_string( &selector, SDB_SHARDINGKEY_NAME, "" ) ;
   rc = sdbbson_finish( &selector ) ;
   if ( SDB_OK != rc )
   {
      ereport( ERROR, ( errcode( ERRCODE_FDW_ERROR ),
            errmsg( "sdbbson_finish failed:rc=%d", rc ) ) ) ;
      goto error ;
   }

   rc = sdbGetSnapshot( connection, SDB_SNAP_CATALOG, &condition, &selector,
         NULL, &cursor ) ;
   if ( SDB_OK != rc )
   {
      if ( rc != SDB_RTN_COORD_ONLY )
      {
         sdbPrintBson( &condition, WARNING, "sdbGetSnapshot" ) ;
         sdbPrintBson( &selector, WARNING, "sdbGetSnapshot" ) ;
         ereport( WARNING, ( errcode( ERRCODE_FDW_ERROR ),
                  errmsg( "sdbGetSnapshot failed:rc=%d", rc ) ) ) ;
      }
      goto error ;
   }

   rc = sdbNext( cursor, &ShardingKey ) ;
   if( rc )
   {
      goto error ;
   }

   type = sdbbson_find( &ite, &ShardingKey, SDB_SHARDINGKEY_NAME ) ;
   if ( BSON_OBJECT == type )
   {
      sdbbson tmpValue ;
      sdbbson_init( &tmpValue ) ;
      sdbbson_iterator_subobject( &ite, &tmpValue ) ;
      sdbbson_iterator_init( &ite, &tmpValue ) ;
      while ( sdbbson_iterator_next( &ite ) )
      {
         const CHAR *sdbbsonKey = sdbbson_iterator_key( &ite ) ;

         strncpy( clStat->shardingKeys[clStat->keyNum],
               sdbbsonKey, SDB_MAX_KEY_COLUMN_LENGTH - 1 ) ;
         clStat->keyNum++ ;
      }
      sdbbson_destroy( &tmpValue ) ;
   }

   sdbCloseCursor( cursor ) ;
done:
   sdbbson_destroy( &condition ) ;
   sdbbson_destroy( &selector ) ;
   sdbbson_destroy( &ShardingKey ) ;
   return rc ;
error:
   goto done ;
}

bool sdbIsShardingKeyChanged( SdbExecState *fdw_state, sdbbson *oldBson,
      sdbbson *newBson )
{
   int i = 1 ;
   sdbbson_iterator oldIte ;
   sdbbson oldSubBson ;
   sdbbson_type oldType ;
   int oldSize ;

   sdbbson_iterator newIte ;
   sdbbson newSubBson ;
   sdbbson_type newType ;
   int newSize ;

   /* shardingKey start from 1( 0 is for the "_id" )*/
   for ( i = 1 ; i < fdw_state->key_num ; i++ )
   {
      sdbbson_init( &newSubBson ) ;
      newType = sdbbson_find( &newIte, newBson, fdw_state->key_name[i] ) ;
      if ( BSON_EOO == newType )
      {
         /* not in the newBson, so no change */
         continue ;
      }
      sdbbson_append_element( &newSubBson, NULL, &newIte ) ;
      sdbbson_finish( &newSubBson ) ;

      sdbbson_init( &oldSubBson ) ;
      oldType = sdbbson_find( &oldIte, oldBson, fdw_state->key_name[i] ) ;
      if ( BSON_EOO == oldType )
      {
         /* not in the oldBson, but appear in the newBson,
            this means the shardingKey is changed */
         ereport( WARNING, ( errcode( ERRCODE_FDW_ERROR ),
               errmsg( "the shardingKey is changed, shardingKey:" ) ) ) ;
         sdbPrintBson( &newSubBson, WARNING, "sdbIsShardingKeyChanged" ) ;
         sdbbson_destroy( &newSubBson ) ;
         sdbbson_destroy( &oldSubBson ) ;
         return true ;
      }
      sdbbson_append_element( &oldSubBson, NULL, &oldIte ) ;
      sdbbson_finish( &oldSubBson ) ;

      oldSize = sdbbson_size( &oldSubBson ) ;
      newSize = sdbbson_size( &newSubBson ) ;
      if ( ( oldSize != newSize )||
            ( 0 != memcmp( newSubBson.data, oldSubBson.data, oldSize ) ) )
      {
         ereport( WARNING, ( errcode( ERRCODE_FDW_ERROR ),
               errmsg( "the shardingKey is changed, old shardingKey:" ) ) ) ;
         sdbPrintBson( &oldSubBson, WARNING, "sdbIsShardingKeyChanged" ) ;
         sdbbson_destroy( &oldSubBson ) ;

         ereport( WARNING, ( errcode( ERRCODE_FDW_ERROR ),
               errmsg( "the shardingKey is changed, new shardingKey:" ) ) ) ;
         sdbPrintBson( &newSubBson, WARNING, "sdbIsShardingKeyChanged" ) ;
         sdbbson_destroy( &newSubBson ) ;

         return true ;
      }

      sdbbson_destroy( &newSubBson ) ;
      sdbbson_destroy( &oldSubBson ) ;
   }

   return false ;
}

UINT64 sdbbson_iterator_getusecs( sdbbson_iterator *ite )
{
   sdbbson_type type = sdbbson_iterator_type( ite ) ;
   if ( BSON_DATE == type )
   {
      return ( sdbbson_iterator_date( ite )* SDB_MSECS_TO_USECS ) ;
   }
   else
   {
      sdbbson_timestamp_t timestamp = sdbbson_iterator_timestamp( ite ) ;
      return ( ( timestamp.t * SDB_SECONDS_TO_USECS )+ timestamp.i ) ;
   }
}

BOOLEAN isNormalChar( CHAR c )
{
   if ( c >= '0' && c <= '9' )
   {
      return TRUE ;
   }
   else if ( c >= 'A' && c <= 'Z' )
   {
      return TRUE ;
   }
   else if ( c >= 'a' && c <= 'z' )
   {
      return TRUE ;
   }
   else
   {
      return FALSE ;
   }
}

CHAR *changeToRegexFormat( CHAR *likeStr )
{
   INT32 i = 0 ;
   INT32 regexj = 0 ;
   CHAR *regexStr  = NULL ;
   INT32 oldlength = strlen( likeStr ) ;
   INT32 newLength = 2 * oldlength + 2 + 1 ;

   regexStr = palloc0( newLength ) ;
   regexStr[regexj] = '^' ;
   regexj++ ;

   for ( i = 0 ; i < oldlength ; i++ )
   {
      if ( isNormalChar( likeStr[i] ) )
      {
         regexStr[regexj] = likeStr[i] ;
         regexj++ ;
      }
      else if ( '_' == likeStr[i] )
      {
         regexStr[regexj] = '.' ;
         regexj++ ;
      }
      else if ( '%' == likeStr[i] )
      {
         regexStr[regexj] = '.' ;
         regexj++ ;
         regexStr[regexj] = '*' ;
         regexj++ ;
      }
      else
      {
         elog( DEBUG1, "can't support like str=[%s]", likeStr ) ;
         goto error ;
      }
   }

   regexStr[regexj] = '$' ;

done:
   return regexStr ;
error:
   pfree( regexStr ) ;
   regexStr = NULL ;
   goto done ;
}

INT32 sdbOperExprParamVar( OpExpr *expr, SdbExprTreeState *expr_state,
                           sdbbson *condition, ExprContext *exprContext )
{
   INT32 rc               = SDB_OK ;
   char *pgOpName         = NULL ;
   const CHAR *sdbOpName  = NULL ;
   char *columnName       = NULL ;
   ListCell *argumentCell = NULL ;
   Var *var               = NULL ;
   Param *param           = NULL ;
   INT32 index            = 0 ;
   INT32 paramIndex       = 0 ;
   INT32 varIndex         = 0 ;
   ParamExecData *paramData = NULL ;


   if ( NULL == exprContext )
   {
      rc = SDB_INVALIDARG ;
      elog(DEBUG1, "exprContext can't be null") ;
      goto error ;
   }

   foreach( argumentCell, expr->args )
   {
      Expr *tmp = (Expr *)lfirst(argumentCell) ;
      index++ ;
      if ( T_Var == nodeTag(tmp) )
      {
         var = (Var *)tmp ;
         if ( var->varno != expr_state->foreign_table_index
              || var->varlevelsup != 0 )
         {
            elog( DEBUG1, "##column is not reconigzed:table_index=%d, varno=%d, "
               "valevelsup=%d", expr_state->foreign_table_index,
               var->varno, var->varlevelsup ) ;
         }

         if ( NUMERICOID == var->vartype && !expr_state->is_use_decimal )
         {
            rc = SDB_INVALIDARG ;
            elog( DEBUG1, "exist decimal condition" ) ;
            goto error ;
         }

         varIndex = index ;
      }
      else if ( T_Param == nodeTag(tmp) )
      {
         param      = (Param *)tmp ;
         paramIndex = index ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         elog( DEBUG1, "unreconigzed expr:type=%d", nodeTag(tmp) ) ;
         goto error ;
      }
   }

   if ( NULL == var || NULL == param )
   {
      rc = SDB_INVALIDARG ;
      elog(DEBUG1, "miss argument") ;
      goto error ;
   }

   if ( PARAM_EXEC != param->paramkind )
   {
      rc = SDB_INVALIDARG ;
      elog( DEBUG1, "paramkind error:paramkind=%d", param->paramkind ) ;
      goto error ;
   }

   pgOpName    = get_opname( expr->opno ) ;
   sdbOpName   = sdbOperatorName( pgOpName, ( varIndex <= paramIndex ) ) ;
   if( !sdbOpName )
   {
      rc = SDB_INVALIDARG ;
      elog( DEBUG1, "operator is not supported1:op=%s", pgOpName ) ;
      goto error ;
   }

   columnName = get_relid_attribute_name( expr_state->foreign_table_id,
                                          var->varattno ) ;
   paramData = &exprContext->ecxt_param_exec_vals[param->paramid] ;

   sdbbson_append_start_object( condition, columnName ) ;
   rc = sdbSetBsonValue( condition, sdbOpName, paramData->value,
                         var->vartype, var->vartypmod,
                         expr_state->is_use_decimal ) ;
   sdbbson_append_finish_object( condition ) ;
   if ( SDB_OK != rc )
   {
      ereport ( WARNING, ( errcode( ERRCODE_FDW_INVALID_DATA_TYPE ),
                errmsg( "convert value failed:key=%s", columnName ) ) ) ;
   }

done:
   return rc ;
error:
   goto done ;
}

Var *getRealVar(Var *arg)
{
   if (NULL == arg)
   {
      return NULL;
   }

   if (T_Var == nodeTag(arg))
   {
      return arg;
   }
   else if (T_RelabelType == nodeTag(arg))
   {
      RelabelType* rtype = (RelabelType *)arg;
      if (NULL == rtype->arg)
      {
         return NULL;
      }

      if (T_Var == nodeTag(rtype->arg))
      {
         return (Var *)rtype->arg;
      }
      else
      {
         elog(DEBUG1, "unreconigzed RelabelType's arg nodeType:nodeTag[%d]",
              nodeTag(rtype->arg));
         return NULL;
      }
   }
   else
   {
      elog(DEBUG1, "unreconigzed nodeType:nodeTag[%d]", nodeTag(arg));
      return NULL;
   }
}

INT32 sdbOperExprTwoVar( OpExpr *opr_two_argument, SdbExprTreeState *expr_state,
                         sdbbson *condition )
{
   INT32 rc               = SDB_OK ;
   char *pgOpName         = NULL ;
   const CHAR *sdbOpName  = NULL ;
   char *columnName1      = NULL ;
   char *columnName2      = NULL ;
   ListCell *argumentCell = NULL ;
   Var *argument1         = NULL ;
   Var *argument2         = NULL ;
   INT32 count            = 0 ;

   sdbbson temp1 ;
   sdbbson temp2 ;

   foreach( argumentCell, opr_two_argument->args )
   {
      if ( count == 0 )
      {
         argument1 = getRealVar((Var *)lfirst(argumentCell));
         if (NULL == argument1)
         {
            elog(DEBUG1, "argument1 is NULL") ;
            goto error ;
         }

         if ( ( argument1->varno != expr_state->foreign_table_index )
          || ( argument1->varlevelsup != 0 ) )
         {
            elog( DEBUG1, "column is not reconigzed:table_index=%d, varno=%d, "
                  "valevelsup=%d", expr_state->foreign_table_index,
                  argument1->varno, argument1->varlevelsup ) ;
            goto error ;
         }

         if ( NUMERICOID == argument1->vartype && !expr_state->is_use_decimal )
         {
            rc = SDB_INVALIDARG ;
            elog( DEBUG1, "exist decimal condition" ) ;
            goto error ;
         }
      }
      else
      {
         argument2 = getRealVar((Var *)lfirst(argumentCell));
         if (NULL == argument2)
         {
            elog(DEBUG1, "argument2 is NULL") ;
            goto error ;
         }

         if ( ( argument2->varno != expr_state->foreign_table_index )
                   || ( argument2->varlevelsup != 0 ) )
         {
            rc = SDB_INVALIDARG ;
            elog( DEBUG1, "column is not reconigzed:table_index=%d, varno=%d, "
                  "valevelsup=%d", expr_state->foreign_table_index,
                  argument2->varno, argument2->varlevelsup ) ;
            goto error ;
         }

         if ( NUMERICOID == argument2->vartype && !expr_state->is_use_decimal )
         {
            rc = SDB_INVALIDARG ;
            elog( DEBUG1, "exist decimal condition" ) ;
            goto error ;
         }
      }

      count++ ;
   }

   /* the caller make sure the argument have two var! */
   pgOpName    = get_opname( opr_two_argument->opno ) ;
   sdbOpName   = sdbOperatorName( pgOpName, TRUE ) ;
   if( !sdbOpName )
   {
      rc = SDB_INVALIDARG ;
      elog( DEBUG1, "operator is not supported2:op=%s", pgOpName ) ;
      goto error ;
   }

   if ( strcmp( sdbOpName, "$regex" ) == 0 )
   {
      rc = SDB_INVALIDARG ;
      elog( DEBUG1, "$regex is not supported(a:{$regex:{$field:'a'}}})") ;
      goto error ;
   }

   columnName1 = get_relid_attribute_name( expr_state->foreign_table_id,
                                                argument1->varattno ) ;
   columnName2 = get_relid_attribute_name( expr_state->foreign_table_id,
                                                argument2->varattno ) ;

   sdbbson_init( &temp1 ) ;
   sdbbson_append_string( &temp1, "$field", columnName2 ) ;
   sdbbson_finish( &temp1 ) ;

   sdbbson_init( &temp2 ) ;
   sdbbson_append_sdbbson( &temp2, sdbOpName, &temp1 ) ;
   sdbbson_finish( &temp2 ) ;
   sdbbson_destroy( &temp1 ) ;

   sdbbson_append_sdbbson( condition, columnName1, &temp2 ) ;
   sdbbson_destroy( &temp2 ) ;

done:
   return rc ;
error:
   goto done ;

}

INT32 sdbOperExpr( OpExpr *opr, SdbExprTreeState *expr_state,
                   sdbbson *condition, ExprContext *exprContext )
{
   INT32 rc              = SDB_OK ;
   char *pgOpName        = NULL ;
   const CHAR *sdbOpName = NULL ;
   char *columnName      = NULL ;
   Var *var              = NULL ;
   Const *const_val      = NULL ;
   bool need_not_condition = false ;
   EnumSdbArgType argType ;

   argType = getArgumentType(opr->args) ;
   if ( SDB_UNSUPPORT_ARG_TYPE == argType )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( SDB_VAR_VAR == argType )
   {
      rc = sdbOperExprTwoVar( opr, expr_state, condition ) ;
      if ( rc != SDB_OK )
      {
         goto error ;
      }

      goto done ;
   }
   else if ( SDB_PARAM_VAR == argType )
   {
      rc = sdbOperExprParamVar( opr, expr_state, condition, exprContext ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      goto done ;
   }
   else if ( SDB_VAR_CONST == argType )
   {
      INT32 varIndex   = 0 ;
      INT32 constIndex = 0 ;
      pgOpName  = get_opname( opr->opno ) ;

      /* first find the key( column )*/
      var = ( Var * )sdbFindArgumentOfType( opr->args, T_Var, &varIndex ) ;
      if ( NULL == var )
      {
         /* var must exist */
         rc = SDB_INVALIDARG ;
         elog( DEBUG1, " Var is null " ) ;
         goto error ;
      }
      elog( DEBUG1, "var info:foreign_table_id=%d, varattno=%d, valevelsup=%d",
                    expr_state->foreign_table_id, var->varattno, var->varlevelsup ) ;
      if ( ( var->varno != expr_state->foreign_table_index )
             || ( var->varlevelsup != 0 ) )
      {
         rc = SDB_INVALIDARG ;
         elog( DEBUG1, "column is not reconigzed:table_index=%d, varno=%d, valevelsup=%d",
               expr_state->foreign_table_index, var->varno, var->varlevelsup ) ;
         goto error ;
      }

      if ( NUMERICOID == var->vartype && !expr_state->is_use_decimal )
      {
         rc = SDB_INVALIDARG ;
         elog( DEBUG1, "exist decimal condition" ) ;
         goto error ;
      }

      columnName = get_relid_attribute_name( expr_state->foreign_table_id,
                                             var->varattno ) ;

      const_val = ( Const * )sdbFindArgumentOfType( opr->args, T_Const,
                                                    &constIndex ) ;
      if ( NULL == const_val ||
           ( InvalidOid != get_element_type( const_val->consttype ) ) )
      {
         /* const must exist and const is not array */
         /* get_element_type(  )!= InvalidOid indicate const_val is array */
         rc = SDB_INVALIDARG ;
         elog( DEBUG1, " const_val is null " ) ;
         goto error ;
      }

      /* we only support > >= < <= <> = ~~ */
      sdbOpName = sdbOperatorName( pgOpName, ( varIndex <= constIndex ) ) ;
      if( !sdbOpName )
      {
         rc = SDB_INVALIDARG ;
         elog( DEBUG1, "operator is not supported3:op=%s", pgOpName ) ;
         goto error ;
      }

      sdbbson_append_start_object( condition, columnName ) ;
      rc = sdbAppendConstantValue( condition, sdbOpName, const_val,
                                   expr_state->is_use_decimal ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_INVALIDARG ;
         elog( DEBUG1, "operator is not supported4:op=%s", pgOpName ) ;
         goto error ;
      }

      sdbbson_append_finish_object( condition ) ;

      if ( need_not_condition )
      {
         sdbbson temp ;
         sdbbson_init( &temp ) ;
         sdbbson_finish( condition ) ;
         sdbbson_copy( &temp, condition ) ;

         sdbbson_destroy( condition ) ;
         sdbbson_init( condition ) ;

         sdbbson_append_start_array( condition, "$not" ) ;
         sdbbson_append_sdbbson( condition, "0", &temp ) ;
         sdbbson_append_finish_array( condition ) ;
         sdbbson_destroy( &temp ) ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 sdbScalarArrayOpExpr( ScalarArrayOpExpr *scalaExpr,
                            SdbExprTreeState *expr_state,
                            sdbbson *condition )
{
   INT32 rc         = SDB_OK ;
   char *pgOpName   = NULL ;
   char *keyName    = NULL ;
   Var *var         = NULL ;
   char *columnName = NULL ;
   ListCell *cell   = NULL ;
   int varCount     = 0 ;
   int constCount   = 0 ;

   pgOpName = get_opname( scalaExpr->opno ) ;
   if ( strcmp( pgOpName, "=" )== 0 )
   {
      keyName = "$in" ;
   }
   else if ( strcmp( pgOpName, "<>" )== 0 )
   {
      keyName = "$nin" ;
   }
   else
   {
      rc = SDB_INVALIDARG ;
      elog( DEBUG1, "operator is not supported5:op=%d,str=%s",
            scalaExpr->opno, pgOpName ) ;
      goto error ;
   }

   /* first find the key( column )*/
   var = ( Var * )sdbFindArgumentOfType( scalaExpr->args, T_Var, NULL ) ;
   if ( NULL == var )
   {
      /* var must exist */
      rc = SDB_INVALIDARG ;
      elog( DEBUG1, " Var is null " ) ;
      goto error ;
   }

   if ( ( var->varno != expr_state->foreign_table_index )
          || ( var->varlevelsup != 0 ) )
   {
      rc = SDB_INVALIDARG ;
      elog( DEBUG1, "column is not reconigzed:table_index=%d, varno=%d, valevelsup=%d",
            expr_state->foreign_table_index, var->varno, var->varlevelsup ) ;
      goto error ;
   }

   if ( NUMERICOID == var->vartype && !expr_state->is_use_decimal )
   {
      rc = SDB_INVALIDARG ;
      elog( DEBUG1, "exist decimal condition" ) ;
      goto error ;
   }

   columnName = get_relid_attribute_name( expr_state->foreign_table_id,
                                          var->varattno ) ;
   foreach( cell, scalaExpr->args )
   {
      Node *oprarg = lfirst( cell ) ;
      if ( T_Const == oprarg->type )
      {
         sdbbson temp ;
         Const *const_val = ( Const * )oprarg ;
         sdbbson_init( &temp ) ;
         rc = sdbSetBsonValue( &temp, keyName, const_val->constvalue,
                               const_val->consttype, const_val->consttypmod,
                               expr_state->is_use_decimal ) ;
         if ( SDB_OK != rc )
         {
            sdbbson_destroy( &temp ) ;
            ereport ( WARNING, ( errcode( ERRCODE_FDW_INVALID_DATA_TYPE ),
                      errmsg( "convert value failed:key=%s", keyName ) ) ) ;
            goto error ;
         }
         sdbbson_finish( &temp ) ;
         sdbbson_append_sdbbson( condition, columnName, &temp ) ;
         sdbbson_destroy( &temp ) ;

         constCount++ ;
      }
      else
      {
         varCount++ ;
      }
   }

   if ( ( varCount != 1 )|| ( constCount != 1 ) )
   {
      rc = SDB_INVALIDARG ;
      elog( DEBUG1, "parameter count error:varCount=%d, constCount=%d",
            varCount, constCount ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 sdbVarExpr( Var *var, SdbExprTreeState *expr_state,
                  sdbbson *condition )
{
   INT32 rc         = SDB_OK ;
   char *columnName = NULL ;
   AttrNumber columnId ;

   if ( ( var->varno != expr_state->foreign_table_index )
          || ( var->varlevelsup != 0 ) )
   {
      rc = SDB_INVALIDARG ;
      elog( DEBUG1, "column is not reconigzed:table_index=%d, varno=%d, valevelsup=%d",
            expr_state->foreign_table_index, var->varno, var->varlevelsup ) ;
      goto error ;
   }

   columnId = var->varattno ;
   columnName = get_relid_attribute_name( expr_state->foreign_table_id,
                                          columnId ) ;
   sdbbson_append_bool( condition, columnName, TRUE ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 sdbBooleanTestIsNotExpr( const char *columnName, sdbbson *condition,
                               BOOLEAN value )
{
   INT32 rc = SDB_OK ;
   sdbbson tmpCondition ;
   sdbbson subCondition ;
   sdbbson_init( &subCondition ) ;
   sdbbson_init( &tmpCondition ) ;

   rc = sdbbson_append_bool( &subCondition, columnName, value ) ;
   if ( SDB_OK != rc )
   {
      elog( WARNING, "sdbbson_append_bool failed:rc=%d", rc ) ;
      goto error ;
   }

   rc = sdbbson_finish( &subCondition ) ;
   if ( SDB_OK != rc )
   {
      elog( WARNING, "sdbbson_finish subCondition failed:rc=%d", rc ) ;
      goto error ;
   }

   rc = sdbbson_append_start_array( &tmpCondition, "$not" ) ;
   if ( SDB_OK != rc )
   {
      elog( WARNING, "sdbbson_append_start_array failed:rc=%d", rc ) ;
      goto error ;
   }

   rc = sdbbson_append_sdbbson( &tmpCondition, "0", &subCondition ) ;
   if ( SDB_OK != rc )
   {
      elog( WARNING, "sdbbson_append_sdbbson failed:rc=%d", rc ) ;
      goto error ;
   }

   rc = sdbbson_append_finish_array( &tmpCondition ) ;
   if ( SDB_OK != rc )
   {
      elog( WARNING, "sdbbson_append_finish_array failed:rc=%d", rc ) ;
      goto error ;
   }

   rc = sdbbson_finish( &tmpCondition ) ;
   if ( SDB_OK != rc )
   {
      elog( WARNING, "sdbbson_finish failed:rc=%d", rc ) ;
      goto error ;
   }

   rc = sdbbson_append_elements( condition, &tmpCondition ) ;
   if ( SDB_OK != rc )
   {
      elog( WARNING, "sdbbson_append_elements failed:rc=%d", rc ) ;
      goto error ;
   }

done:
   sdbbson_destroy( &subCondition ) ;
   sdbbson_destroy( &tmpCondition ) ;

   return rc ;
error:
   goto done ;
}

INT32 sdbBooleanTestExpr( BooleanTest *boolTest, SdbExprTreeState *expr_state,
                          sdbbson *condition )
{
   INT32 rc         = SDB_OK ;
   char *columnName = NULL ;
   Node *node       = NULL ;
   Var *var         = NULL ;
   AttrNumber columnId ;

   node = ( Node * )boolTest->arg ;
   if ( NULL == node )
   {
      rc = SDB_INVALIDARG ;
      elog( DEBUG1, "boolTest'a arg is NULL" ) ;
      goto error;
   }

   if ( !IsA( node, Var ) )
   {
      rc = SDB_INVALIDARG ;
      elog( DEBUG1, "boolTest'a arg must be Var type" ) ;
      goto error;
   }

   var = ( Var *) node ;

   if ( ( var->varno != expr_state->foreign_table_index )
          || ( var->varlevelsup != 0 ) )
   {
      rc = SDB_INVALIDARG ;
      elog( DEBUG1, "column is not reconigzed:table_index=%d, varno=%d, valevelsup=%d",
            expr_state->foreign_table_index, var->varno, var->varlevelsup ) ;
      goto error ;
   }

   columnId = var->varattno ;
   columnName = get_relid_attribute_name( expr_state->foreign_table_id,
                                          columnId ) ;

   if ( IS_TRUE == boolTest->booltesttype )
   {
      sdbbson_append_bool( condition, columnName, TRUE ) ;
   }
   else if ( IS_FALSE == boolTest->booltesttype )
   {
      sdbbson_append_bool( condition, columnName, FALSE ) ;
   }
   else if ( IS_NOT_TRUE == boolTest->booltesttype )
   {
      rc = sdbBooleanTestIsNotExpr( columnName, condition, TRUE );
      if ( SDB_OK != rc )
      {
         elog( DEBUG1, "sdbBooleanTestIsNotExpr TRUE failed:rc=%d", rc ) ;
         goto error ;
      }
   }
   else if ( IS_NOT_FALSE == boolTest->booltesttype )
   {
      rc = sdbBooleanTestIsNotExpr( columnName, condition, FALSE );
      if ( SDB_OK != rc )
      {
         elog( DEBUG1, "sdbBooleanTestIsNotExpr FALSE failed:rc=%d", rc ) ;
         goto error ;
      }
   }
   else
   {
      rc = SDB_INVALIDARG ;
      elog( DEBUG1, "unreconigzed booltesttype:type=%d",
            boolTest->booltesttype ) ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 sdbNullTestExpr( NullTest *ntest, SdbExprTreeState *expr_state,
                       sdbbson *condition )
{
   INT32 rc         = SDB_OK ;
   char *columnName = NULL ;
   Var *var         = NULL ;
   sdbbson isNullcondition ;
   AttrNumber columnId ;

   if ( ntest->argisrow )
   {
      rc = SDB_INVALIDARG ;
      elog( DEBUG1, "argisrow is true" ) ;
      goto error ;
   }

   if ( T_Var != ( ( Expr * )( ntest->arg ) )->type )
   {
      rc = SDB_INVALIDARG ;
      elog( DEBUG1, "argument is not var:type=%d", ( ( Expr * )( ntest->arg ) )->type ) ;
      goto error ;
   }

   var = ( Var * )ntest->arg ;
   if ( ( var->varno != expr_state->foreign_table_index )
          || ( var->varlevelsup != 0 ) )
   {
      rc = SDB_INVALIDARG ;
      elog( DEBUG1, "column is not reconigzed:table_index=%d, varno=%d, valevelsup=%d",
            expr_state->foreign_table_index, var->varno, var->varlevelsup ) ;
      goto error ;
   }

   columnId = var->varattno ;
   columnName    = get_relid_attribute_name( expr_state->foreign_table_id,
                                                 columnId ) ;
   sdbbson_init( &isNullcondition ) ;
   switch ( ntest->nulltesttype )
   {
      case IS_NULL:
         sdbbson_append_int( &isNullcondition, "$isnull", 1 ) ;
         break ;
      case IS_NOT_NULL:
         sdbbson_append_int( &isNullcondition, "$isnull", 0 ) ;

         break ;
      default:
         rc = SDB_INVALIDARG ;
         sdbbson_destroy( &isNullcondition ) ;
         elog( DEBUG1, "nulltesttype error:type=%d", ntest->nulltesttype ) ;
         goto error ;
   }

   sdbbson_finish( &isNullcondition ) ;
   sdbbson_append_sdbbson( condition, columnName, &isNullcondition ) ;
   sdbbson_destroy( &isNullcondition ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 sdbRecurBoolExpr( BoolExpr *boolexpr, SdbExprTreeState *expr_state,
                        sdbbson *condition, ExprContext *exprContext )
{
   INT32 rc = SDB_OK ;
   ListCell *cell ;
   char *key = NULL ;
   sdbbson tmpCondition ;
   SdbExprTreeState tmp_expr_state ;
   int count = 0 ;

   switch ( boolexpr->boolop )
   {
      case AND_EXPR:
         key = "$and" ;
         break ;

      case OR_EXPR:
         key = "$or" ;
         break ;

      case NOT_EXPR:
         key = "$not" ;
         break ;
      default:
         rc = SDB_INVALIDARG ;
         elog( DEBUG1, "unsupported boolean expression type:type=%d",
               boolexpr->boolop ) ;
         goto error ;
   }

   memset( &tmp_expr_state, 0, sizeof(tmp_expr_state) ) ;
   tmp_expr_state.foreign_table_id      = expr_state->foreign_table_id ;
   tmp_expr_state.foreign_table_index   = expr_state->foreign_table_index ;
   tmp_expr_state.is_use_decimal        = expr_state->is_use_decimal ;

   sdbbson_init( &tmpCondition ) ;
   sdbbson_append_start_array( &tmpCondition, key ) ;
   foreach( cell, boolexpr->args )
   {
      INT32 rcTmp = SDB_OK ;
      Node *bool_arg = ( Node * )lfirst ( cell ) ;
      sdbbson sub_condition ;
      sdbbson_init( &sub_condition ) ;
      rcTmp = sdbRecurExprTree( bool_arg, &tmp_expr_state, &sub_condition,
                                exprContext ) ;
      sdbbson_finish( &sub_condition ) ;

      if ( SDB_OK == rcTmp )
      {
         if ( !sdbbson_is_empty( &sub_condition ) )
         {
            CHAR arrayIndex[SDB_MAX_KEY_COLUMN_LENGTH + 1] = "" ;
            sprintf( arrayIndex, "%d", count ) ;
            sdbbson_append_sdbbson( &tmpCondition, arrayIndex,
                                    &sub_condition ) ;
            count++ ;
         }
      }
      else
      {
         tmp_expr_state.total_unsupport_count++ ;
         if ( 0 == strcmp( key, "$and" ) )
         {
            tmp_expr_state.and_unsupport_count++ ;
         }
         else
         {
            tmp_expr_state.or_unsupport_count++ ;
         }
      }

      sdbbson_destroy( &sub_condition ) ;
   }

   sdbbson_append_finish_array( &tmpCondition ) ;
   sdbbson_finish( &tmpCondition ) ;
   if ( 0 == tmp_expr_state.total_unsupport_count ||
        0 == strcmp( key, "$and" ) )
   {
      if ( count > 0 )
      {
         sdbbson_append_elements( condition, &tmpCondition ) ;
      }
   }
   else // tmp_expr_state.total_unsupport_count > && 0 != strcmp( key, "$and" )
   {
      if ( 0 == strcmp( key, "$or" ) && 0 == tmp_expr_state.or_unsupport_count )
      {
         if ( count > 0 )
         {
            sdbbson_append_elements( condition, &tmpCondition ) ;
         }
      }
   }

   sdbbson_destroy( &tmpCondition ) ;
   expr_state->total_unsupport_count += tmp_expr_state.total_unsupport_count ;
   expr_state->and_unsupport_count   += tmp_expr_state.and_unsupport_count ;
   if ( 0 != strcmp( key, "$and" ) )
   {
      expr_state->or_unsupport_count += tmp_expr_state.or_unsupport_count ;
   }
   else
   {
      /*
         do not deliver the or_unsupport_count if we meet the AND
         let's assume C is unsupported.
         A or ( B and ( C or D ) )  => A or B
      */
   }

done:
   return rc ;

error:
   goto done ;
}

/* This is a recurse function to walk over the tree */
INT32 sdbRecurExprTree( Node *node, SdbExprTreeState *expr_state,
                        sdbbson *condition, ExprContext *exprContext )
{
   INT32 rc = SDB_OK ;
   if( NULL == node )
   {
      goto done ;
   }

   if( IsA( node, BoolExpr ) )
   {
      rc = sdbRecurBoolExpr( ( BoolExpr * )node, expr_state, condition,
                             exprContext ) ;
      if ( rc != SDB_OK )
      {
         elog( DEBUG1, "sdbRecurBoolExpr" ) ;
         goto error ;
      }
   }
   else if ( IsA( node, NullTest ) )
   {
      rc = sdbNullTestExpr( ( NullTest * )node, expr_state, condition ) ;
      if ( rc != SDB_OK )
      {
         elog( DEBUG1, "sdbRecurNullTestExpr" ) ;
         goto error ;
      }
   }
   /*ScalarArrayOpExpr*/
   else if ( IsA( node, ScalarArrayOpExpr ) )
   {
      rc = sdbScalarArrayOpExpr( ( ScalarArrayOpExpr * )node,
                                 expr_state, condition ) ;
      if ( rc != SDB_OK )
      {
         elog( DEBUG1, "sdbRecurScalarArrayOpExpr" ) ;
         goto error ;
      }
   }
   else if ( IsA( node, OpExpr ) )
   {
      rc = sdbOperExpr( ( OpExpr * )node, expr_state, condition, exprContext ) ;
      if ( rc != SDB_OK )
      {
         elog( DEBUG1, "sdbRecurOperExpr" ) ;
         goto error ;
      }
   }
   else if ( IsA( node, BooleanTest ) )
   {
      rc = sdbBooleanTestExpr( ( BooleanTest * )node, expr_state, condition );
      if ( rc != SDB_OK )
      {
         elog( DEBUG1, "sdbBooleanTestExpr failed:rc=%d", rc ) ;
         goto error ;
      }
   }
   else if ( IsA( node, Var ) )
   {
      rc = sdbVarExpr( ( Var * )node, expr_state, condition );
      if ( rc != SDB_OK )
      {
         elog( DEBUG1, "sdbVarExpr failed:rc=%d", rc ) ;
         goto error ;
      }
   }
   else
   {
      rc = SDB_INVALIDARG ;
      elog( DEBUG1, "node type is not supported:type=%d", nodeTag( node ) ) ;
      goto error ;
   }

done:
   return rc ;

error:
   goto done ;

}

INT32 sdbGenerateFilterCondition ( Oid foreign_id, RelOptInfo *baserel,
                                   INT32 isUseDecimal, sdbbson *condition )
{
   ListCell *cell = NULL ;
   INT32 rc       = SDB_OK ;
   SdbExprTreeState expr_state ;
   BOOLEAN isExistUnsupport = FALSE ;

   memset( &expr_state, 0, sizeof( SdbExprTreeState ) ) ;
   expr_state.foreign_table_index = baserel->relid ;
   expr_state.foreign_table_id    = foreign_id ;
   expr_state.is_use_decimal      = isUseDecimal ;


   sdbbson_destroy( condition ) ;
   sdbbson_init( condition ) ;
   foreach( cell, baserel->baserestrictinfo )
   {
      INT32 rcTmp = SDB_OK ;
      sdbbson subBson ;
      RestrictInfo *info =( RestrictInfo * )lfirst( cell ) ;
      sdbbson_init( &subBson ) ;
      expr_state.total_unsupport_count = 0 ;
      rcTmp = sdbRecurExprTree( ( Node * )info->clause, &expr_state, &subBson,
                                NULL ) ;
      sdbbson_finish( &subBson ) ;
      if( SDB_OK == rcTmp )
      {
         if ( !sdbbson_is_empty( &subBson ) )
         {
            sdbbson_append_elements( condition, &subBson ) ;
         }
      }

      sdbbson_destroy( &subBson ) ;
      if ( expr_state.total_unsupport_count > 0 )
      {
         isExistUnsupport = TRUE ;
      }
   }

   rc = sdbbson_finish( condition ) ;
   if ( SDB_OK != rc )
   {
      sdbbson_destroy( condition ) ;
      sdbbson_init( condition ) ;
      sdbbson_finish( condition ) ;
   }

   if ( isExistUnsupport )
   {
      return -1 ;
   }

   return SDB_OK ;
}

static void sdbInitConnectionPool (  )
{
   SdbConnection *pNewMem  = NULL ;
   SdbConnectionPool *pool = sdbGetConnectionPool(  ) ;
   memset( pool, 0, sizeof( SdbConnectionPool ) ) ;
   pNewMem = ( SdbConnection* )malloc( sizeof( SdbConnection )*
                                      INITIAL_ARRAY_CAPACITY ) ;
   if( !pNewMem )
   {
      ereport( ERROR,( errcode( ERRCODE_FDW_OUT_OF_MEMORY ),
                         errmsg( "Unable to allocate connection pool" ),
                         errhint( "Make sure the memory pool or ulimit is "
                                   "properly configured" ) ) ) ;
      goto error ;
   }
   pool->connList = pNewMem ;
   pool->poolSize = INITIAL_ARRAY_CAPACITY ;
error :
   return ;
}

static void sdbUninitConnectionPool (  )
{
   INT32 count             = 0 ;
   SdbConnectionPool *pool = sdbGetConnectionPool(  ) ;
   for( count = 0 ; count < pool->numConnections ; ++count )
   {
      SdbConnection *conn = &pool->connList[count] ;
      if( conn->connName )
      {
         free( conn->connName ) ;
      }
      sdbDisconnect( conn->hConnection ) ;
      sdbReleaseConnection( conn->hConnection ) ;
   }
   free( pool->connList ) ;
}

static void sdbInterruptAllConnection()
{
   INT32 count             = 0 ;
   SdbConnectionPool *pool = sdbGetConnectionPool() ;
   for( count = 0 ; count < pool->numConnections ; ++count )
   {
      SdbConnection *conn = &pool->connList[count] ;
      sdbDisconnect( conn->hConnection ) ;
   }
}



/* sdbSerializeDocument serializes the sdbbson document to a constant
 * Note this function just copies the pointer of documents' data,
 * therefore the caller should NOT destroy the object
 */
Const *sdbSerializeDocument( sdbbson *document )
{
   Const *serializedDocument = NULL ;
   const CHAR *documentData  = sdbbson_data( document ) ;
   INT32 documentSize        = sdbbson_buffer_size( document ) ;
   if ( NULL != documentData )
   {
      Datum documentDatum = 0 ;
      CHAR *pTmp = palloc( documentSize + 1 ) ;
      memcpy( pTmp, documentData, documentSize ) ;
      pTmp[ documentSize ] = '\0' ;
      documentDatum      = CStringGetDatum( pTmp ) ;
      serializedDocument = makeConst( CSTRINGOID, -1, InvalidOid, documentSize,
                                      documentDatum, FALSE, FALSE ) ;
   }
   else
   {
      serializedDocument = makeNullConst( TEXTOID, -1, InvalidOid ) ;
   }

   return serializedDocument ;
}

/* sdbDeserializeDocument deserializes a constant into sdbbson document
 */
void sdbDeserializeDocument( Const *constant, sdbbson *document )
{
   if ( constant->constisnull )
   {
      sdbbson_init( document ) ;
      sdbbson_finish( document ) ;
   }
   else
   {
      Datum documentDatum = constant->constvalue ;
      CHAR *documentData = DatumGetCString( documentDatum ) ;
      sdbbson_init_size( document, 0 ) ;
      sdbbson_init_finished_data( document, documentData ) ;
   }

   return ;
}

/* sdbOperatorName converts PG comparison operator to Sdb
 */
const CHAR *sdbOperatorName( const CHAR *operatorName, BOOLEAN isVarFirst )
{
   const CHAR *pResult                  = NULL ;
   const INT32 totalNames               = 7 ;
   static const CHAR *nameMappings[][3] = {
      { "<",   "$lt",  "$gt"   },
      { "<=",  "$lte", "$gte"  },
      { ">",   "$gt",  "$lt"   },
      { ">=",  "$gte", "$lte"  },
      { "=",   "$et",  "$et"   },
      { "<>",  "$ne",  "$ne"   },
      { "~~",  "$regex", "$regex" }
   } ;

   INT32 i = 0 ;
   for( i = 0 ; i < totalNames ; ++i )
   {
      if( strncmp( nameMappings[i][0],
                     operatorName, NAMEDATALEN )== 0 )
      {
         if ( isVarFirst )
         {
            pResult = nameMappings[i][1] ;
         }
         else
         {
            pResult = nameMappings[i][2] ;
         }

         break ;
      }
   }
   return pResult ;
}

/* sdbFindArgumentOfType iterate the given argument list, looks for an argument
 * with the given type and returns the argument if found
 */
Expr *sdbFindArgumentOfType( List *argumentList, NodeTag argumentType,
                             INT32 *argIndex )
{
   Expr *foundArgument    = NULL ;
   ListCell *argumentCell = NULL ;
   INT32 index = 0 ;
   foreach( argumentCell, argumentList )
   {
      Expr *argument = ( Expr * )lfirst( argumentCell ) ;
      index++ ;
      if( nodeTag( argument )== argumentType )
      {
         foundArgument = argument ;
         if ( NULL != argIndex )
         {
            *argIndex = index ;
         }
         break ;
      }
      else if ( nodeTag( argument ) == T_RelabelType )
      {
         RelabelType *relabel = (RelabelType *)argument ;
         if ( nodeTag( relabel->arg ) == argumentType )
         {
            foundArgument = relabel->arg ;
            if ( NULL != argIndex )
            {
               *argIndex = index ;
            }
            break ;
         }
      }
   }
   return foundArgument ;
}

/* sdbAppendConstantValue appends to query document with key and value */
INT32 sdbAppendConstantValue ( sdbbson *bsonObj, const char *keyName,
                               Const *constant, INT32 isUseDecimal )
{
   int rc = SDB_OK ;

   if ( constant->constisnull )
   {
      /* this matches null and not exists for both table and index scan */
      sdbbson_append_int( bsonObj, "$isnull", 1 ) ;
      return rc ;
   }

   rc = sdbSetBsonValue( bsonObj, keyName, constant->constvalue,
                         constant->consttype, constant->consttypmod,
                         isUseDecimal ) ;
   if ( SDB_OK != rc )
   {
      ereport ( WARNING, ( errcode( ERRCODE_FDW_INVALID_DATA_TYPE ),
                errmsg( "convert value failed:key=%s", keyName ) ) ) ;
   }

   return rc ;
}

/* sdbApplicableOpExpressionList iterate all operators and push the predicates
 * that's able to handled by sdb into sdb
 */



/* sdbColumnList takes the planner's information and extract all columns that
 * may be used in projections, joins, and filter clauses, de-duplicate those
 * columns and returns them in a new list
 */
static List *sdbColumnList( RelOptInfo *baserel )
{
   List *columnList           = NIL ;
   List *neededColumnList     = NIL ;
   AttrNumber columnIndex     = 1 ;
   AttrNumber columnCount     = baserel->max_attr ;
   List *targetColumnList     = baserel->reltargetlist ;
   List *restrictInfoList     = baserel->baserestrictinfo ;
   ListCell *restrictInfoCell = NULL ;
   /* first add the columns used in joins and projections */
   neededColumnList = list_copy( targetColumnList ) ;
   /* then walk over all restriction clauses, and pull up any used columns */
   foreach( restrictInfoCell, restrictInfoList )
   {
      RestrictInfo *restrictInfo = ( RestrictInfo * )lfirst( restrictInfoCell ) ;
      Node *restrictClause       = ( Node * )restrictInfo->clause ;
      List *clauseColumnList     = NIL ;
      /* recursively pull up any columns used in the restriction clauses */
      clauseColumnList = pull_var_clause( restrictClause ,
                                           PVC_RECURSE_AGGREGATES,
                                           PVC_RECURSE_PLACEHOLDERS ) ;
      neededColumnList = list_union( neededColumnList, clauseColumnList ) ;
   }
   /* walk over all column definitions and deduplicate column list */
   for ( columnIndex = 1 ; columnIndex <= columnCount ; columnIndex++ )
   {
      ListCell *neededColumnCell = NULL ;
      Var *column = NULL ;
      foreach( neededColumnCell, neededColumnList )
      {
         Var *neededColumn = ( Var * )lfirst( neededColumnCell ) ;
         if( neededColumn->varattno == columnIndex )
         {
            column = neededColumn ;
            break ;
         }
      }
      if( NULL != column )
      {
         columnList = lappend( columnList, column ) ;
      }
   }
   return columnList ;
}

/* Iterate SdbInputOptionList and return all possible option name for
 * the given context id
 */
static StringInfo sdbListOptions( const Oid currentContextID )
{
   StringInfo resultString = makeStringInfo (  ) ;
   INT32 firstPrinted      = 0 ;
   INT32 i                 = 0 ;
   for( i = 0 ; i < sizeof( SdbInputOptionList ) ; ++i )
   {
      const SdbInputOption *inputOption = &( SdbInputOptionList[i] ) ;
      /* if currentContextID matches the context, then let's append */
      if( currentContextID == inputOption->optionContextID )
      {
         /* append ", " if it's not the first element */
         if( firstPrinted )
         {
            appendStringInfoString( resultString, ", " ) ;
         }
         appendStringInfoString( resultString, inputOption->optionName ) ;
         firstPrinted = 1 ;
      }
   }
   return resultString ;
}

/* sdbGetOptionValue retrieve options info from PG catalog */
static CHAR *sdbGetOptionValue( Oid foreignTableId, const CHAR *optionName )
{
   ForeignTable *ft     = NULL ;
   ForeignServer *fs    = NULL ;
   List *optionList     = NIL ;
   ListCell *optionCell = NULL ;
   CHAR *optionValue    = NULL ;
   /* retreive foreign table and server in order to get options */
   ft  = GetForeignTable( foreignTableId ) ;
   fs = GetForeignServer( ft->serverid ) ;
   /* add server and table options into list */
   optionList    = list_concat( optionList, ft->options ) ;
   optionList    = list_concat( optionList, fs->options ) ;
   /* iterate each option and match the option name */
   foreach( optionCell, optionList )
   {
      DefElem *optionDef = ( DefElem * )lfirst( optionCell ) ;
      if( strncmp( optionDef->defname, optionName, NAMEDATALEN )== 0 )
      {
         optionValue = defGetString( optionDef ) ;
         goto done ;
      }
   }
done :
   return optionValue ;
}


/* getOptions retreive connection options from Oid */
void sdbGetOptions( Oid foreignTableId, SdbInputOptions *options )
{
   CHAR *addressName         = NULL ;
   CHAR *serviceName         = NULL ;
   CHAR *userName            = NULL ;
   CHAR *passwordName        = NULL ;
   CHAR *collectionspaceName = NULL ;
   CHAR *collectionName      = NULL ;
   CHAR *preferedInstance    = NULL ;
   CHAR *transaction         = NULL ;
   CHAR *pUseDecimal       = NULL ;
   INT32 isUseDecimal        = 0 ;
   if( NULL == options )
      goto done ;
   /* address name */
   addressName = sdbGetOptionValue( foreignTableId,
                                     OPTION_NAME_ADDRESS ) ;
   if( NULL == addressName )
   {
      addressName = pstrdup( DEFAULT_HOSTNAME ) ;
   }

   /* service name */
   serviceName = sdbGetOptionValue( foreignTableId,
                                     OPTION_NAME_SERVICE ) ;
   if( NULL == serviceName )
   {
      serviceName = pstrdup( DEFAULT_SERVICENAME ) ;
   }

   /* user name */
   userName = sdbGetOptionValue( foreignTableId,
                                 OPTION_NAME_USER ) ;
   if( NULL == userName )
   {
      userName = pstrdup( DEFAULT_USERNAME ) ;
   }

   /* password name */
   passwordName = sdbGetOptionValue( foreignTableId,
                                     OPTION_NAME_PASSWORD ) ;
   if( NULL == passwordName )
   {
      passwordName = pstrdup( DEFAULT_PASSWORDNAME ) ;
   }

   /* collectionspace name */
   collectionspaceName = sdbGetOptionValue( foreignTableId,
                                            OPTION_NAME_COLLECTIONSPACE ) ;
   if( NULL == collectionspaceName )
   {
      /* if collectionspace name is not provided, let's use the schema name
       * of the table
       */
      collectionspaceName = get_namespace_name (
            get_rel_namespace( foreignTableId ) ) ;
   }

   /* collection name */
   collectionName = sdbGetOptionValue( foreignTableId,
                                       OPTION_NAME_COLLECTION ) ;
   if( NULL == collectionName )
   {
      /* if collection name is not provided, let's use the table name */
      collectionName = get_rel_name( foreignTableId ) ;
   }

   /* OPTION_NAME_PREFEREDINSTANCE */
   preferedInstance = sdbGetOptionValue( foreignTableId,
                                         OPTION_NAME_PREFEREDINSTANCE ) ;
   if ( NULL == preferedInstance )
   {
      preferedInstance = pstrdup( DEFAULT_PREFEREDINSTANCE ) ;
   }

   /* OPTION_NAME_TRANSACTION */
   transaction = sdbGetOptionValue( foreignTableId, OPTION_NAME_TRANSACTION ) ;
   if ( NULL == transaction )
   {
      transaction = pstrdup( DEFAULT_TRANSACTION ) ;
   }

   /* OPTION_NAME_USEDECIMAL */
   pUseDecimal = sdbGetOptionValue( foreignTableId, OPTION_NAME_USEDECIMAL ) ;
   if ( NULL == pUseDecimal )
   {
      pUseDecimal = pstrdup( DEFAULT_DECIMAL ) ;
   }
   if ( strcmp( pUseDecimal, SDB_DECIMAL_ON ) == 0 )
   {
      isUseDecimal = 1 ;
   }
   else
   {
      isUseDecimal = 0 ;
   }

   /* fill up the result structure */
   SdbSetNodeAddressInfo( options, addressName, serviceName ) ;

   options->user                = userName ;
   options->password            = passwordName ;
   options->collectionspace     = collectionspaceName ;
   options->collection          = collectionName ;
   options->preference_instance = preferedInstance ;
   options->transaction         = transaction ;
   options->isUseDecimal        = isUseDecimal ;

done :
   return ;
}

/* validator validates for:
 * foreign data wrapper
 * server
 * user mapping
 * foreign table
 */
Datum sdb_fdw_validator( PG_FUNCTION_ARGS )
{
   Datum optionArray    = PG_GETARG_DATUM( 0 ) ;
   Oid optionContextId  = PG_GETARG_OID( 1 ) ;
   List *optionList     = untransformRelOptions( optionArray ) ;
   ListCell *optionCell = NULL ;

   /* iterate each element in option */
   foreach( optionCell, optionList )
   {
      DefElem *option  =( DefElem * )lfirst( optionCell ) ;
      CHAR *optionName = option->defname ;
      INT32 i = 0 ;
      /* find out which option it is */
      for( i = 0 ;
            i < sizeof( SdbInputOptionList )/ sizeof( SdbInputOption ) ;
            ++i )
      {
         const SdbInputOption *inputOption = &( SdbInputOptionList[i] ) ;
         /* compare type and name */
         if( optionContextId == inputOption->optionContextID &&
              strncmp( optionName, inputOption->optionName,
                        NAMEDATALEN )== 0 )
         {
            /* if we find the option, let's jump out */
            break ;
         }
      }
      /* if we don't find any match */
      if( sizeof( SdbInputOptionList )/ sizeof( SdbInputOption )== i )
      {
         StringInfo optionNames = sdbListOptions( optionContextId ) ;
         ereport( ERROR,( errcode( ERRCODE_FDW_INVALID_OPTION_NAME ),
                            errmsg( "invalid option \"%s\"", optionName ),
                            errhint( "Valid options in this context are: %s",
                                      optionNames->data ) ) ) ;
      }
      /* make sure the port is integer */
      if( strncmp( optionName, OPTION_NAME_SERVICE, NAMEDATALEN )== 0 )
      {
         CHAR *optionValue = defGetString( option ) ;
         INT32 portNumber  = pg_atoi( optionValue, sizeof( INT32 ), 0 ) ;
         ( void )portNumber ;
      }
   }
   PG_RETURN_VOID (  ) ;
}

/* sdbColumnMappingHash creates a hash table to map sdbbson fields into PG column
 * index
 */
static HTAB *sdbColumnMappingHash( Oid foreignTableId,
                                    List *columnList )
{
   ListCell *columnCell     = NULL ;
   const long hashTableSize = 2048 ;
   HTAB *columnMappingHash  = NULL ;
   /* create hash table */
   HASHCTL hashInfo ;
   memset( &hashInfo, 0, sizeof( hashInfo ) ) ;
   hashInfo.keysize = NAMEDATALEN ;
   hashInfo.entrysize = sizeof( SdbColumnMapping ) ;
   hashInfo.hash = string_hash ;
   hashInfo.hcxt = CurrentMemoryContext ;

   /* create hash table */
   columnMappingHash = hash_create( "Column Mapping Hash",
                                     hashTableSize,
                                     &hashInfo,
                                     ( HASH_ELEM | HASH_FUNCTION | HASH_CONTEXT )
                                  ) ;
   if( !columnMappingHash )
   {
      goto error ;
   }
   /* iterate each column */
   foreach( columnCell, columnList )
   {
      Var *column                     = ( Var* )lfirst( columnCell ) ;
      AttrNumber columnId             = column->varattno ;
      SdbColumnMapping *columnMapping = NULL ;
      CHAR *columnName                = NULL ;
      bool handleFound                = false ;
      void *hashKey                   = NULL ;

      columnName = get_relid_attribute_name( foreignTableId, columnId ) ;
      hashKey = ( void * )columnName ;

      /* validate each column only appears once, HASH_ENTER means we want to
       * enter the entry into hash table if not found
       */
      columnMapping = ( SdbColumnMapping* )hash_search( columnMappingHash,
                                                       hashKey,
                                                       HASH_ENTER,
                                                       &handleFound ) ;
      if( !columnMapping )
      {
         goto error ;
      }
      columnMapping->columnIndex       = columnId - 1 ;
      columnMapping->columnTypeId      = column->vartype ;
      columnMapping->columnTypeMod     = column->vartypmod ;
      columnMapping->columnArrayTypeId = get_element_type( column->vartype ) ;
   }
done :
   return columnMappingHash ;
error :
   goto done ;
}

/* sdbColumnTypesCompatible checks if a sdbbson type is compatible with PG type */
static BOOLEAN sdbColumnTypesCompatible( sdbbson_type sdbbsonType,
                                         Oid columnTypeId, INT32 isUseDecimal )
{
   BOOLEAN compatibleType = FALSE ;
   switch( columnTypeId )
   {
   case BOOLOID :
      if( BSON_BOOL == sdbbsonType )
      {
         compatibleType = TRUE ;
      }
      /* do not break here since we also want to compatible with int/long/double
       * for boolean type
       */
   case INT2OID :
   case INT4OID :
   case INT8OID :
   case FLOAT4OID :
   case FLOAT8OID :
   {
      if( BSON_INT == sdbbsonType || BSON_LONG == sdbbsonType ||
           BSON_DOUBLE == sdbbsonType )
      {
         compatibleType = TRUE ;
      }
      break;
   }
   case NUMERICOID :
   {
      if ( !isUseDecimal )
      {
         if( BSON_STRING == sdbbsonType || BSON_DOUBLE == sdbbsonType ||
             BSON_INT == sdbbsonType || BSON_LONG == sdbbsonType )
         {
            compatibleType = TRUE ;
         }
      }
      else
      {
         if ( BSON_DECIMAL == sdbbsonType )
         {
            compatibleType = TRUE ;
         }
      }

      break ;
   }
   case BPCHAROID :
   case VARCHAROID :
   case TEXTOID :
   {
      if( BSON_STRING == sdbbsonType )
         compatibleType = TRUE ;
      break ;
   }
   case NAMEOID :
   {
      if( BSON_OID == sdbbsonType )
         compatibleType = TRUE ;
      break ;
   }
   case DATEOID :
   case TIMESTAMPOID :
   case TIMESTAMPTZOID :
   {
      if ( ( BSON_TIMESTAMP == sdbbsonType )|| ( BSON_DATE == sdbbsonType ) )
         compatibleType = TRUE ;
      break ;
   }
   case BYTEAOID :
   {
      if ( ( BSON_BINDATA == sdbbsonType ) )
      {
         compatibleType = TRUE ;
      }
      break ;
   }
   case JSONOID :
   {
      compatibleType = TRUE ;
      break ;
   }
   default :
   {
      ereport( WARNING,( errcode( ERRCODE_FDW_INVALID_DATA_TYPE ),
                       errmsg( "cannot convert sdbbson type to column type" ),
                       errhint( "Column type: %u",
                                 ( UINT32 )columnTypeId ) ) ) ;
      break ;
   }
   }
   return compatibleType ;
}

/* sdbColumnValue converts sdbbson value into PG datum
 */
static Datum sdbColumnValue( sdbbson_iterator *sdbbsonIterator, Oid columnTypeId,
                             INT32 columnTypeMod )
{
   Datum columnValue = 0 ;
   switch( columnTypeId )
   {
   case INT2OID :
   {
      INT16 value = ( INT16 )sdbbson_iterator_int( sdbbsonIterator ) ;
      columnValue = Int16GetDatum( value ) ;
      break ;
   }
   case INT4OID :
   {
      INT32 value = sdbbson_iterator_int( sdbbsonIterator ) ;
      columnValue = Int32GetDatum( value ) ;
      break ;
   }
   case INT8OID :
   {
      INT64 value = sdbbson_iterator_long( sdbbsonIterator ) ;
      columnValue = Int64GetDatum( value ) ;
      break ;
   }
   case FLOAT4OID :
   {
      FLOAT32 value = ( FLOAT32 )sdbbson_iterator_double( sdbbsonIterator ) ;
      columnValue = Float4GetDatum( value ) ;
      break ;
   }
   case FLOAT8OID :
   {
      FLOAT64 value = sdbbson_iterator_double( sdbbsonIterator ) ;
      columnValue = Float8GetDatum( value ) ;
      break ;
   }
   case NUMERICOID :
   {
      sdbbson_type bsonType = sdbbson_iterator_type(sdbbsonIterator);
      if ( bsonType == BSON_STRING )
      {
         const char *value = sdbbson_iterator_string( sdbbsonIterator ) ;
         Datum valueStr    = CStringGetDatum( value ) ;
         columnValue       = DirectFunctionCall3( numeric_in,
                             valueStr, ObjectIdGetDatum(InvalidOid),
                             Int32GetDatum(columnTypeMod) );
      }
      else if ( bsonType == BSON_DECIMAL )
      {
         INT32 rc    = SDB_OK ;
         INT32 size  = 0 ;
         char *value = NULL ;
         Datum valueStr ;
         sdbbson_decimal decimal ;
         decimal_init( &decimal ) ;
         rc = sdbbson_iterator_decimal( sdbbsonIterator, &decimal ) ;
         if ( SDB_OK != rc )
         {
            decimal_free( &decimal ) ;
            elog( ERROR, "get decimal faild:rc=%d", rc ) ;
            break ;
         }

         decimal_to_str_get_len( &decimal, &size ) ;
         value = palloc( size + 1 );
         rc    = decimal_to_str( &decimal, value, size ) ;
         if ( SDB_OK != rc )
         {
            decimal_free( &decimal ) ;
            elog( ERROR, "decimal to str faild:rc=%d", rc ) ;
            break ;
         }

         if (columnTypeMod >= (int32) (VARHDRSZ))
         {
            int tmpTypeMod = columnTypeMod - VARHDRSZ ;
            if ( decimal_is_out_of_precision( &decimal, tmpTypeMod ) )
            {
               int precision = (tmpTypeMod >> 16) & 0xffff;
               int scale     = tmpTypeMod & 0xffff;
               decimal_free( &decimal ) ;
               elog( ERROR, "value[%s] is out of precision[%d,%d]",
                     value, precision, scale ) ;
               break ;
            }
         }

         decimal_free( &decimal ) ;
         valueStr    = CStringGetDatum( value ) ;
         columnValue = DirectFunctionCall3( numeric_in,
                                         valueStr, ObjectIdGetDatum(InvalidOid),
                                         Int32GetDatum(columnTypeMod) );
      }
      else
      {
         Datum numberNoTypeMode;
         FLOAT64 value    = sdbbson_iterator_double( sdbbsonIterator ) ;
         Datum valueDatum = Float8GetDatum( value ) ;
         numberNoTypeMode = DirectFunctionCall1( float8_numeric, valueDatum ) ;
         columnValue      = DirectFunctionCall2( numeric, numberNoTypeMode,
                                                 columnTypeMod ) ;
      }

      break ;
   }
   case BOOLOID :
   {
      BOOLEAN value = sdbbson_iterator_bool( sdbbsonIterator ) ;
      columnValue = BoolGetDatum( value ) ;
      break ;
   }
   case BPCHAROID :
   {
      const CHAR *value = sdbbson_iterator_string( sdbbsonIterator ) ;
      Datum valueDatum = CStringGetDatum( value ) ;
      columnValue = DirectFunctionCall3( bpcharin, valueDatum,
                                          ObjectIdGetDatum( InvalidOid ),
                                          Int32GetDatum( columnTypeMod ) ) ;
      break ;
   }
   case VARCHAROID :
   {
      const CHAR *value = sdbbson_iterator_string( sdbbsonIterator ) ;
      Datum valueDatum = CStringGetDatum( value ) ;
      columnValue = DirectFunctionCall3( varcharin, valueDatum,
                                          ObjectIdGetDatum( InvalidOid ),
                                          Int32GetDatum( columnTypeMod ) ) ;
      break ;
   }
   case TEXTOID :
   {
      const CHAR *value = sdbbson_iterator_string( sdbbsonIterator ) ;
      columnValue       = CStringGetTextDatum( value ) ;
      break ;
   }
   case NAMEOID :
   {
      CHAR value [ NAMEDATALEN ] = {0} ;
      Datum valueDatum = 0 ;
      sdbbson_oid_t *sdbbsonObjectId = sdbbson_iterator_oid( sdbbsonIterator ) ;
      sdbbson_oid_to_string( sdbbsonObjectId, value ) ;
      valueDatum               = CStringGetDatum( value ) ;
      columnValue = DirectFunctionCall3( namein, valueDatum,
                                          ObjectIdGetDatum( InvalidOid ),
                                          Int32GetDatum( columnTypeMod ) ) ;
      break ;
   }
   case DATEOID :
   {
      INT64 utcUsecs       = sdbbson_iterator_getusecs( sdbbsonIterator ) ;
      INT64 timestamp      = utcUsecs - POSTGRES_TO_UNIX_EPOCH_USECS ;
      Datum timestampDatum = TimestampGetDatum( timestamp ) ;
      columnValue = DirectFunctionCall1( timestamptz_date, timestampDatum ) ;
      break ;
   }
   case TIMESTAMPOID :
   case TIMESTAMPTZOID :
   {
      INT64 utcUsecs       = sdbbson_iterator_getusecs( sdbbsonIterator ) ;
      INT64 timestamp      = utcUsecs - POSTGRES_TO_UNIX_EPOCH_USECS ;
      Datum timestampDatum = TimestampGetDatum( timestamp ) ;
      columnValue = DirectFunctionCall1( timestamptz_timestamp, timestampDatum ) ;
      break ;
   }
   case BYTEAOID :
   {
      const CHAR *buff = sdbbson_iterator_bin_data( sdbbsonIterator ) ;
      INT32 len        = sdbbson_iterator_bin_len( sdbbsonIterator ) ;

      bytea *result = (bytea *)palloc( len + VARHDRSZ ) ;
      memcpy( VARDATA(result), buff, len ) ;
      SET_VARSIZE(result, len + VARHDRSZ) ;

      columnValue = PointerGetDatum(result) ;

      break ;
   }
   case JSONOID :
   {
      INT32 length = 0 ;
      CHAR *pBuff  = NULL ;
      CHAR *pTemp  = NULL ;
      Datum tmpResult ;

      length = sdbbson_sprint_length_iterator( sdbbsonIterator ) ;
      pBuff = palloc0( length + 1 ) ;
      if ( NULL == pBuff )
      {
         ereport( ERROR, ( errcode( ERRCODE_FDW_ERROR ),
                  errmsg( "out of memory" ) ) ) ;
      }
      pTemp = pBuff ;
      sdbbson_sprint_iterator( &pTemp, &length, sdbbsonIterator, '"' ) ;
      tmpResult = CStringGetTextDatum( pBuff ) ;
      columnValue = PointerGetDatum( tmpResult ) ;

      break ;
   }
   default :
   {
      ereport( ERROR,( errcode( ERRCODE_FDW_INVALID_DATA_TYPE ),
                         errmsg( "cannot convert sdbbson type to column type" ),
                         errhint( "Column type: %u",
                                   ( UINT32 )columnTypeId ) ) ) ;
      break ;
   }
   }
   return columnValue ;
}

/* sdbFreeScanStateInModifyEnd closes the cursor, connection and collection to SequoiaDB
 */
void sdbFreeScanStateInModifyEnd( SdbExecState *executionState )
{
   if( !executionState )
      goto done ;

   if( SDB_INVALID_HANDLE != executionState->hCursor )
      sdbReleaseCursor( executionState->hCursor ) ;

   if( SDB_INVALID_HANDLE != executionState->hCollection )
   {
      sdbReleaseCollection( executionState->hCollection ) ;
   }
done :
   return ;
}


/* sdbFreeScanState closes the cursor, connection and collection to SequoiaDB
 */
void sdbFreeScanState( SdbExecState *executionState, bool deleteShared )
{
   if( !executionState )
      goto done ;

   if( SDB_INVALID_HANDLE != executionState->hCursor )
      sdbReleaseCursor( executionState->hCursor ) ;

   executionState->hCollection = SDB_INVALID_HANDLE ;
   if ( deleteShared )
   {
      if ( -1 != executionState->bson_record_addr )
      {
         SdbReleaseRecord( SdbGetRecordCache(),
                           executionState->bson_record_addr ) ;
         executionState->bson_record_addr = -1 ;
      }
   }
   /* do not free connection since it's in pool */
done :
   return ;
}

/* sdbColumnValueArray build array
 */
static Datum sdbColumnValueArray( sdbbson_iterator *sdbbsonIterator,
                                   Oid valueTypeId, INT32 isUseDecimal )
{
   UINT32 arrayCapacity          = INITIAL_ARRAY_CAPACITY ;
   UINT32 arrayGrowthFactor      = 2 ;
   UINT32 arrayIndex             = 0 ;
   ArrayType *columnValueObject  = NULL ;
   Datum columnValueDatum        =  0 ;
   bool typeByValue              = false ;
   CHAR typeAlignment            = 0 ;
   INT16 typeLength              = 0 ;
   sdbbson_iterator sdbbsonSubIterator = { NULL, 0 } ;
   Datum *columnValueArray       = NULL ;
   sdbbson_iterator_subiterator( sdbbsonIterator, &sdbbsonSubIterator ) ;
   columnValueArray = ( Datum* )palloc0( INITIAL_ARRAY_CAPACITY *
                                        sizeof( Datum ) ) ;
   if( !columnValueArray )
   {
      ereport( ERROR,( errcode( ERRCODE_FDW_OUT_OF_MEMORY ),
                         errmsg( "Unable to allocate array memory" ),
                         errhint( "Make sure the memory pool or ulimit is "
                                   "properly configured" ) ) ) ;
      goto error ;
   }
   /* go through each element in array */
   while( sdbbson_iterator_next( &sdbbsonSubIterator ) )
   {
      sdbbson_type sdbbsonType = sdbbson_iterator_type( &sdbbsonSubIterator ) ;
      BOOLEAN compatibleTypes = FALSE ;
      if ( BSON_ARRAY == sdbbsonType && OidIsValid( valueTypeId ) )
      {
         compatibleTypes = TRUE ;
      }
      else
      {
         compatibleTypes = sdbColumnTypesCompatible( sdbbsonType, valueTypeId,
                                                     isUseDecimal ) ;
      }
      /* skip the element if it's not compatible */
      if( BSON_NULL == sdbbsonType || !compatibleTypes )
      {
         continue ;
      }
      if( arrayIndex >= arrayCapacity )
      {
         arrayCapacity *= arrayGrowthFactor ;
         columnValueArray = repalloc( columnValueArray,
                                       arrayCapacity * sizeof( Datum ) ) ;
      }
      /* use default type modifier to convert column value */
/*
      if ( BSON_ARRAY == sdbbsonType )
      {
         columnValueArray[arrayIndex] = sdbColumnValueArray( &sdbbsonSubIterator,
                                                              valueTypeId ) ;
      }
      else
      {
         columnValueArray[arrayIndex] = sdbColumnValue( &sdbbsonSubIterator,
                                                         valueTypeId, 0 ) ;
      }
*/
      columnValueArray[arrayIndex] = sdbColumnValue( &sdbbsonSubIterator,
                                                     valueTypeId, 0 ) ;
      ++arrayIndex ;
   }
done :
   get_typlenbyvalalign( valueTypeId, &typeLength, &typeByValue,
                          &typeAlignment ) ;
   columnValueObject = construct_array( columnValueArray, arrayIndex,
                                         valueTypeId, typeLength,
                                         typeByValue, typeAlignment ) ;
   columnValueDatum = PointerGetDatum( columnValueObject ) ;
   return columnValueDatum ;
error :
   goto done ;
}

/* sdbFillTupleSlot go through sdbbson document and the hash table, try to build
 * the tuple for PG
 * documentKey: if we want to find things in nested sdbbson object
 */
static void sdbFillTupleSlot( const sdbbson *sdbbsonDocument,
                              const CHAR *documentKey,
                              HTAB *columnMappingHash,
                              INT32 isUseDecimal,
                              Datum *columnValues,
                              bool *columnNulls )
{
   sdbbson_iterator sdbbsonIterator = { NULL, 0 } ;
   sdbbson_iterator_init( &sdbbsonIterator, sdbbsonDocument ) ;

   /* for each element in sdbbson object */
   while( sdbbson_iterator_next( &sdbbsonIterator ) )
   {
      const CHAR *sdbbsonKey   = sdbbson_iterator_key( &sdbbsonIterator ) ;
      sdbbson_type sdbbsonType = sdbbson_iterator_type( &sdbbsonIterator ) ;
      SdbColumnMapping *columnMapping = NULL ;
      Oid columnTypeId                = InvalidOid ;
      Oid columnArrayTypeId           = InvalidOid ;
      BOOLEAN compatibleTypes         = FALSE ;
      bool handleFound                = false ;
      const CHAR *sdbbsonFullKey         = NULL ;
      void *hashKey                   = NULL ;

      /*
       * if we have object
       * {
       *    a: {
       *       A: 1,
       *       B: 2
       *    },
       *    b: "hello"
       * }
       * first round we have documentKey=NULL, then sdbbsonKey=a
       * next we have nested object, then recursively we call sdbFillTupleSlot
       * so we have
       * documentKey=a, sdbbsonKey=A, then we have sdbbsonFullKey=a.A
       */
      if( documentKey )
      {
         /* if we want to find entries in nested object, we should use this one
          */
         StringInfo sdbbsonFullKeyString = makeStringInfo (  ) ;
         appendStringInfo( sdbbsonFullKeyString, "%s.%s", documentKey,
                            sdbbsonKey ) ;
         sdbbsonFullKey = sdbbsonFullKeyString->data ;
      }
      else
      {
         sdbbsonFullKey = sdbbsonKey ;
      }

      /* match columns for sdbbson key */
      hashKey = ( void* )sdbbsonFullKey ;
      columnMapping = ( SdbColumnMapping* )hash_search( columnMappingHash,
                                                       hashKey,
                                                       HASH_FIND,
                                                       &handleFound ) ;
      if ( NULL != columnMapping )
      {
         columnTypeId      = columnMapping->columnTypeId ;
         columnArrayTypeId = columnMapping->columnArrayTypeId ;
      }

      /* recurse into nested objects */
      if( BSON_OBJECT == sdbbsonType && JSONOID != columnTypeId )
      {
         sdbbson subObject ;
         sdbbson_init( &subObject ) ;
         sdbbson_iterator_subobject( &sdbbsonIterator, &subObject ) ;
         sdbFillTupleSlot( &subObject, sdbbsonFullKey, columnMappingHash,
                           isUseDecimal, columnValues, columnNulls ) ;
         sdbbson_destroy( &subObject ) ;
         continue ;
      }

      /* if we cannot find the column, or if the sdbbson type is null, let's just
       * leave it as null
       */
      if( NULL == columnMapping || BSON_NULL == sdbbsonType )
      {
         continue ;
      }

      if( OidIsValid( columnArrayTypeId )&& sdbbsonType == BSON_ARRAY )
      {
         compatibleTypes = TRUE ;
      }
      else
      {
         compatibleTypes = sdbColumnTypesCompatible( sdbbsonType, columnTypeId,
                                                     isUseDecimal ) ;
      }

      /* if types are incompatible, leave this column null */
      if( !compatibleTypes )
      {
         continue ;
      }
      /* fill in corresponding column values and null flag */
      if( OidIsValid( columnArrayTypeId ) )
      {
         INT32 columnIndex = columnMapping->columnIndex ;
         columnValues[columnIndex] = sdbColumnValueArray( &sdbbsonIterator,
                                                          columnArrayTypeId,
                                                          isUseDecimal ) ;
         columnNulls[columnIndex] = false ;
      }
      else
      {
         INT32 columnIndex = columnMapping->columnIndex ;
         Oid columnTypeMod = columnMapping->columnTypeMod ;
         columnValues[columnIndex] = sdbColumnValue( &sdbbsonIterator,
                                                     columnTypeId,
                                                     columnTypeMod ) ;
         columnNulls[columnIndex] = false ;
      }
   }
}

static INT32 sdbRowsCount( Oid foreignTableId, SINT64 *count )
{
   INT32 rc = SDB_OK ;
   const SdbCLStatistics *clStat =
               SdbGetCLStatFromCache( foreignTableId );
   if ( NULL != clStat )
   {
      *count = clStat->recordCount ;
   }
   else
   {
      rc = SDB_SYS ;
   }
   return rc ;
}

/* sdbRowsCount get count of records in the given collection */
static INT32 sdbRowsCountFromSdb( Oid foreignTableId, SINT64 *count )
{
   INT32 rc                        = SDB_OK ;
   sdbCollectionHandle hCollection = SDB_INVALID_HANDLE ;
   sdbConnectionHandle hConnection = SDB_INVALID_HANDLE ;
   SdbInputOptions options ;
   StringInfo fullCollectionName = makeStringInfo (  ) ;

   sdbGetOptions( foreignTableId, &options ) ;
   /* attempt to connect to remote database */
   hConnection = sdbGetConnectionHandle( (const char **)options.serviceList,
                                         options.serviceNum, options.user,
                                         options.password,
                                         options.preference_instance,
                                         options.transaction ) ;
   if ( SDB_INVALID_HANDLE == hConnection )
   {
      goto error ;
   }

   /* get collection */
   hCollection = sdbGetSdbCollection( hConnection, options.collectionspace,
            options.collection ) ;
   if ( SDB_INVALID_HANDLE == hCollection )
   {
      ereport( ERROR, ( errcode( ERRCODE_FDW_ERROR ),
            errmsg( "Unable to get collection \"%s.%s\", rc = %d",
            options.collectionspace, options.collection, rc ),
            errhint( "Make sure the collectionspace and "
               "collection exist on the remote database" ) ) ) ;

       goto error ;
   }

   /* get count */
   rc = sdbGetCount( hCollection, NULL, count ) ;
   if( rc )
   {
      ereport( ERROR,( errcode( ERRCODE_FDW_ERROR ),
                         errmsg( "Unable to get count for \"%s\", rc = %d",
                                  fullCollectionName->data, rc ),
                         errhint( "Make sure the collectionspace and "
                                   "collection exist on the remote database" )
                       ) ) ;
      goto error ;
   }
done :
   if( SDB_INVALID_HANDLE != hCollection )
      sdbReleaseCollection( hCollection ) ;
   return rc ;
error :
   goto done ;
}

/* Get the estimation size for Sdb foreign table
 * This function is also responsible to establish connection to remote table
 */
static void SdbGetForeignRelSize( PlannerInfo *root,
                                  RelOptInfo *baserel,
                                  Oid foreignTableId )
{
   INT32 rc                = SDB_OK ;
   SdbExecState *fdw_state = NULL ;
   List *rowClauseList     = NULL ;
   FLOAT64 rowSelectivity  = 0.0 ;
   FLOAT64 outputRowCount  = 0.0 ;

   fdw_state = palloc0( sizeof( SdbExecState ) ) ;
   initSdbExecState( fdw_state ) ;
   sdbGetSdbServerOptions( foreignTableId, fdw_state ) ;
   fdw_state->tableID = foreignTableId ;
   fdw_state->relid   = baserel->relid ;

   fdw_state->pgTableDesc = sdbGetPgTableDesc( foreignTableId ) ;
   if ( NULL == fdw_state->pgTableDesc )
   {
      ereport( ERROR,( errcode( ERRCODE_FDW_OUT_OF_MEMORY ),
            errmsg( "Unable to allocate pgTableDesc" ),
            errhint( "Make sure the memory pool or ulimit is properly configured" ) ) ) ;
      return ;
   }

   rc = sdbRowsCount( foreignTableId, &fdw_state->row_count ) ;
   if ( rc )
   {
      ereport( ERROR, ( errmsg( "Unable to retrieve the row count for collection" ),
            errhint( "Falling back to default estimates in planning" ) ) ) ;
      return ;
   }

   rowClauseList  = baserel->baserestrictinfo ;
   rowSelectivity = clauselist_selectivity( root, rowClauseList, 0, JOIN_INNER, NULL ) ;
   outputRowCount = clamp_row_est( fdw_state->row_count * rowSelectivity ) ;
   baserel->rows  = outputRowCount ;

   baserel->fdw_private = ( void * )fdw_state ;
}

/* SdbGetForeignPaths creates the scan path to execute query.
 * Sdb may decide to use underlying index, but the caller procedures do not
 * know whether it will happen or not.
 * Therefore, we simply create a table scan path
 * Estimation includes:
 * 1 )row count
 * 2 )disk cost
 * 3 )cpu cost
 * 4 )startup cost
 */
static void SdbGetForeignPaths( PlannerInfo *root,
                                 RelOptInfo *baserel,
                                 Oid foreignTableId )
{
   INT32 rc                 = SDB_OK ;
   BlockNumber pageCount    = 0 ;
   INT32 rowWidth           = 0 ;
   FLOAT64 selectivity      = 0.0 ;
   FLOAT64 inputRowCount    = 0.0 ;
   FLOAT64 foreignTableSize = 0.0 ;
   Path *foreignPath        = NULL ;

   /* cost estimation */
   FLOAT64 totalDiskCost    = 0.0 ;
   FLOAT64 totalCPUCost     = 0.0 ;
   FLOAT64 totalStartupCost = 0.0 ;
   FLOAT64 totalCost        = 0.0 ;

   sdbbson condition ;

#ifdef SDB_USE_OWN_POSTGRES

   sdbIndexInfo sdb_idxs[SDB_MAX_INDEX_NUM] ;
   int indexNum = 0 ;
   int i = 0 ;
   sdbIndexClauseSet idxClauseSet ;

#endif /* SDB_USE_OWN_POSTGRES */

   SdbExecState *fdw_state  = NULL ;

   fdw_state = ( SdbExecState * )baserel->fdw_private ;
   /* rows estimation after applying predicates */
   sdbbson_init( &condition ) ;
   sdbbson_finish( &condition ) ;
   rc = sdbGenerateFilterCondition(foreignTableId, baserel,
                                   fdw_state->isUseDecimal, &condition);
   sdbbson_destroy(&condition) ;
   if ( SDB_OK == rc )
   {
      selectivity = clauselist_selectivity( root, baserel->baserestrictinfo,
                                            0, JOIN_INNER, NULL ) ;
   }
   else
   {
      selectivity = clauselist_selectivity( root, NIL,
                                            0, JOIN_INNER, NULL ) ;
   }

   inputRowCount = clamp_row_est( fdw_state->row_count * selectivity ) ;

   /* disk cost estimation */
   rowWidth         = get_relation_data_width( foreignTableId,
                                                baserel->attr_widths ) ;
   foreignTableSize = rowWidth * fdw_state->row_count ;
   pageCount =( BlockNumber )rint( foreignTableSize / BLCKSZ ) ;
   totalDiskCost = seq_page_cost * pageCount ;

   /* cpu cost estimation, it includes the record process time + return time
    * cpu_tuple_cost: time to process each row
    * SDB_TUPLE_COST_MULTIPLIER * cpu_tuple_cost: time to return a row
    * baserel->baserestrictcost.per_tuple: time to process a row in PG
    */
   totalCPUCost = cpu_tuple_cost * fdw_state->row_count +
                 ( SDB_TUPLE_COST_MULTIPLIER * cpu_tuple_cost +
                    baserel->baserestrictcost.per_tuple )* inputRowCount ;

   /* startup cost estimation, which includes query execution startup time
    */
   totalStartupCost = baserel->baserestrictcost.startup ;

   /* total cost includes totalDiskCost + totalCPUCost + totalStartupCost
    */
   totalCost = totalStartupCost + totalCPUCost + totalDiskCost ;

#ifdef SDB_USE_OWN_POSTGRES
   memset(sdb_idxs, 0, sizeof(sdbIndexInfo) * SDB_MAX_INDEX_NUM ) ;
   sdbGetIndexInfos(fdw_state, sdb_idxs, SDB_MAX_INDEX_NUM, &indexNum) ;
   for ( i = 0; i < indexNum; i++ )
   {
      sdbIndexInfo *pSdbIdx = &sdb_idxs[i] ;
      INT32 rcTmp = SDB_OK ;
      sdbbson condition;
      sdbbson_init(&condition) ;
      rcTmp = sdbGenerateFilterCondition(foreignTableId, baserel,
                                         fdw_state->isUseDecimal, &condition) ;
      sdbbson_destroy(&condition) ;
      if ( SDB_OK == rcTmp )
      {
         MemSet(&idxClauseSet, 0, sizeof(idxClauseSet));
         sdbGetIndexEqclause(root, baserel, foreignTableId, pSdbIdx,
                             &idxClauseSet) ;
         if ( idxClauseSet.nonempty )
         {
            Path *idxpath;
            idxpath = (Path* )sdb_build_index_paths(root, baserel, pSdbIdx,
                                                    &idxClauseSet,
                                                    fdw_state);
            if ( NULL != idxpath )
            {
               add_path(baserel, idxpath);
            }
         }
         else
         {
            sdbIndexClauseSet jcClauseSet ;
            MemSet( &jcClauseSet, 0, sizeof(jcClauseSet) ) ;
            sdbMatchJoinClausesToIndex( root, baserel, foreignTableId, pSdbIdx,
                                        &jcClauseSet ) ;
            if ( jcClauseSet.nonempty )
            {
               Path *idxpath;
               idxpath = (Path* )sdb_build_index_paths(root, baserel, pSdbIdx,
                                                       &jcClauseSet,
                                                       fdw_state);
               if ( NULL != idxpath )
               {
                  add_path(baserel, idxpath);
               }
            }
         }
      }

   }

#endif /* SDB_USE_OWN_POSTGRES */

   /* create foreign path node */

   foreignPath = ( Path* )create_foreignscan_path( root, baserel, inputRowCount,
         totalStartupCost, totalCost, NIL, /* no pathkeys */
         NULL, /* no outer rel */
         NIL ) ; /* no fdw_private */

   /* add foreign path into path */
   add_path( baserel, foreignPath ) ;
   return ;
}

/* SdbGetForeignPlan creates a foreign scan plan node for Sdb collection */
static ForeignScan *SdbGetForeignPlan( PlannerInfo *root,
                                       RelOptInfo *baserel,
                                       Oid foreignTableId,
                                       ForeignPath *bestPath,
                                       List *targetList,
                                       List *restrictionClauses )
{
   Index scanRangeTableIndex = baserel->relid ;
   ForeignScan *foreignScan  = NULL ;
   List *foreignPrivateList  = NIL ;
   List *columnList          = NIL ;

   SdbExecState *fdw_state   = ( SdbExecState * )baserel->fdw_private ;
   sdbGetColumnKeyInfo( fdw_state ) ;

   /* We keep all restriction clauses at PG ide to re-check */
   restrictionClauses = extract_actual_clauses( restrictionClauses, FALSE ) ;
   /* construct the query sdbbson document */

   sdbGenerateFilterCondition( foreignTableId, baserel, fdw_state->isUseDecimal,
                               &fdw_state->queryDocument ) ;
   sdbPrintBson( &fdw_state->queryDocument, DEBUG1, "foreignplan" ) ;

   fdw_state->bson_record_addr = sdbCreateBsonRecordAddr() ;
   foreignPrivateList = serializeSdbExecState( fdw_state ) ;
   sdbbson_destroy( &fdw_state->queryDocument ) ;

   /* copy document list */
   columnList = sdbColumnList( baserel ) ;
   /* construct foreign plan with query predicates and column list */
   foreignPrivateList = list_make2( foreignPrivateList, columnList ) ;

   /* create the foreign scan node */
   foreignScan =  make_foreignscan( targetList, restrictionClauses,
                                     scanRangeTableIndex,
                                     NIL, /* no expressions to evaluate */
                                     foreignPrivateList ) ;
   /* object should NOT be destroyed since serializeSdbExecState does not make
    * memory copy
    * So sdbbson_destroy is only called when rc != SDB_OK( that means no
    * serializeSdbExecState is called )
    */
   return foreignScan ;
}

/* SdbExplainForeignScan produces output for EXPLAIN command
 */
static void SdbExplainForeignScan( ForeignScanState *scanState,
                                    ExplainState *explainState )
{
   ForeignScan *foreignScan  = NULL ;
   List *foreignPrivateList  = NIL ;
   List *fdw_state_list      = NIL ;
   SdbExecState *fdw_state   = NULL ;

   SdbInputOptions options ;
   StringInfo namespaceName = makeStringInfo (  ) ;
   Oid foreignTableId = RelationGetRelid( scanState->ss.ss_currentRelation ) ;
   sdbGetOptions( foreignTableId, &options ) ;
   appendStringInfo( namespaceName, "%s.%s",
                      options.collectionspace, options.collection ) ;
   ExplainPropertyText( "Foreign Namespace", namespaceName->data,
                         explainState ) ;

   /* deserialize fdw*/
   foreignScan        = ( ForeignScan * )scanState->ss.ps.plan ;
   foreignPrivateList = foreignScan->fdw_private ;
   fdw_state_list     = ( List * )linitial( foreignPrivateList ) ;
   fdw_state          = deserializeSdbExecState( fdw_state_list ) ;
   if ( -1 != fdw_state->bson_record_addr )
   {
      SdbReleaseRecord( SdbGetRecordCache(), fdw_state->bson_record_addr ) ;
      fdw_state->bson_record_addr = -1 ;
   }

   pfree( fdw_state ) ;
}

/* SdbBeginForeignScan connects to sdb server and open a cursor to perform scan
 */
static void SdbBeginForeignScan( ForeignScanState *scanState,
                                  INT32 executorFlags )
{
   Oid foreignTableId        = InvalidOid ;
   ForeignScan *foreignScan  = NULL ;
   List *foreignPrivateList  = NIL ;
   List *columnList          = NIL ;
   HTAB *columnMappingHash   = NULL ;
   List *fdw_state_list      = NIL ;
   SdbExecState *fdw_state   = NULL ;
   SdbStatisticsCache *cache = NULL ;
   const SdbCLStatistics *clStat = NULL ;

   /* do not begin real scan if it's explain only */
   if( executorFlags & EXEC_FLAG_EXPLAIN_ONLY )
   {
      return ;
   }

   foreignTableId = RelationGetRelid( scanState->ss.ss_currentRelation ) ;
   cache = SdbGetStatisticsCache() ;
   clStat = SdbGetCLStatFromCache( foreignTableId ) ;

   /* deserialize fdw*/
   foreignScan        = ( ForeignScan * )scanState->ss.ps.plan ;
   foreignPrivateList = foreignScan->fdw_private ;
   fdw_state_list     = ( List * )linitial( foreignPrivateList ) ;
   fdw_state          = deserializeSdbExecState( fdw_state_list ) ;

   /* build hash map */
   columnList = ( List * )lsecond( foreignPrivateList ) ;
   columnMappingHash = sdbColumnMappingHash( foreignTableId, columnList ) ;

   fdw_state->planType          = SDB_PLAN_SCAN ;
   fdw_state->columnMappingHash = columnMappingHash ;

   /* retreive target information */
   fdw_state->hConnection = sdbGetConnectionHandle(
                                        (const char **)fdw_state->sdbServerList,
                                        fdw_state->sdbServerNum,
                                        fdw_state->usr,
                                        fdw_state->passwd,
                                        fdw_state->preferenceInstance,
                                        fdw_state->transaction ) ;

   fdw_state->hCollection = clStat->clHandle ;

   scanState->fdw_state = ( void* )fdw_state ;
}

/* SdbIterateForeignScan reads the next record from SequoiaDB and converts into
 * PostgreSQL tuple
 */
static TupleTableSlot * SdbIterateForeignScan( ForeignScanState *scanState )
{
   sdbbson *recordObj           = NULL ;
   INT32 rc                     = SDB_OK ;
   SdbExecState *executionState = ( SdbExecState* )scanState->fdw_state ;
   TupleTableSlot *tupleSlot    = scanState->ss.ss_ScanTupleSlot ;
   TupleDesc tupleDescriptor    = tupleSlot->tts_tupleDescriptor ;
   Datum *columnValues          = tupleSlot->tts_values ;
   bool *columnNulls            = tupleSlot->tts_isnull ;
   INT32 columnCount            = tupleDescriptor->natts ;
   const CHAR *sdbbsonDocumentKey  = NULL ;

   if ( SDB_INVALID_HANDLE == executionState->hCursor )
   {
      rc = sdbQuery1( executionState->hCollection, &executionState->queryDocument, NULL,
                      NULL, NULL, 0, -1, FLG_QUERY_WITH_RETURNDATA,
                      &executionState->hCursor ) ;
      if ( rc )
      {
         sdbPrintBson(&executionState->queryDocument, WARNING, "SdbIterateForeignScan") ;
         ereport( ERROR, ( errcode ( ERRCODE_FDW_ERROR ),
         errmsg( "query collection failed:cs=%s,cl=%s,rc=%d",
                  executionState->sdbcs, executionState->sdbcl, rc ),
         errhint( "Make sure collection exists on remote SequoiaDB database" ) ) ) ;

         sdbbson_dispose( &executionState->queryDocument ) ;
         goto error ;
      }
   }

   recordObj = sdbGetRecordPointer( executionState->bson_record_addr ) ;
   sdbbson_destroy( recordObj ) ;
   sdbbson_init( recordObj ) ;
   /* if there's nothing more to fetch, we return empty slot to represent
    * there's no more data to read
    */
   ExecClearTuple( tupleSlot ) ;
   /* initialize all values */
   memset( columnValues, 0, columnCount * sizeof( Datum ) ) ;
   memset( columnNulls, TRUE, columnCount * sizeof( bool ) ) ;

   /* cursor read next */
   rc = sdbNext( executionState->hCursor, recordObj ) ;
   if( rc )
   {
      if( SDB_DMS_EOC != rc )
      {
         sdbFreeScanState( executionState, true ) ;
         /* if other error happened, let's report them */
         ereport( ERROR,( errcode( ERRCODE_FDW_ERROR ),
                            errmsg( "unable to fetch next record"
                                     ", rc = %d", rc ),
                            errhint( "Make sure collection exists on remote "
                                      "SequoiaDB database, and the server "
                                      "is still up and running" ) ) ) ;
         goto error ;
      }
      /* if we get EOC, let's just goto done to return empty tupleSlot */
      goto done ;
   }

   sdbFillTupleSlot( recordObj, sdbbsonDocumentKey,
                     executionState->columnMappingHash,
                     executionState->isUseDecimal,
                     columnValues, columnNulls ) ;

   ExecStoreVirtualTuple( tupleSlot ) ;
done :

   return tupleSlot ;
error :
   goto done ;
}

/* SdbRescanForeignScan rescans the foreign table
 */
static void SdbRescanForeignScan( ForeignScanState *scanState )
{
   INT32 rc                     = SDB_OK ;
   sdbbson rescanCondition ;
   SdbExecState *executionState =( SdbExecState * )scanState->fdw_state ;
   if( !executionState )
      goto error ;
   /* kill the cursor and start up a new one */
   sdbReleaseCursor( executionState->hCursor ) ;

   sdbbson_init( &rescanCondition ) ;
   rc = sdbGenerateRescanCondition( executionState, &scanState->ss.ps,
                                    &rescanCondition ) ;
   sdbPrintBson( &rescanCondition, DEBUG1, "rescan" ) ;
   if ( SDB_OK == rc )
   {
      rc = sdbQuery1( executionState->hCollection, &rescanCondition, NULL,
                      NULL, NULL, 0, -1, FLG_QUERY_WITH_RETURNDATA,
                      &executionState->hCursor ) ;
   }
   else
   {
      rc = sdbQuery1( executionState->hCollection,
                      &executionState->queryDocument, NULL,
                      NULL, NULL, 0, -1, FLG_QUERY_WITH_RETURNDATA,
                      &executionState->hCursor ) ;
   }

   sdbbson_destroy( &rescanCondition ) ;

   if( rc )
   {
      ereport( ERROR,( errcode( ERRCODE_FDW_ERROR ),
                         errmsg( "unable to rescan collection"
                                  ", rc = %d", rc ),
                         errhint( "Make sure collection exists on remote "
                                   "SequoiaDB database" ) ) ) ;
      goto error ;
   }
done :
   return ;
error :
   goto done ;
}

/* SdbEndForeignScan finish scanning the foreign table
 */
static void SdbEndForeignScan( ForeignScanState *scanState )
{
   SdbExecState *executionState =( SdbExecState * )scanState->fdw_state ;
   if( executionState )
   {
      sdbFreeScanState( executionState, true ) ;
   }
}

/* sdbAcquireSampleRows acquires a random sample of rows from the foreign table.
 */
static INT32 sdbAcquireSampleRows( Relation relation, INT32 errorLevel,
                                    HeapTuple *sampleRows, INT32 targetRowCount,
                                    FLOAT64 *totalRowCount,
                                    FLOAT64 *totalDeadRowCount )
{
   INT32 rc                          = SDB_OK ;
   INT32 sampleRowCount              = 0 ;
   INT64 rowCount                    = 0 ;
   INT64 rowCountToSkip              = -1 ;
   FLOAT64 randomState               = 0 ;
   Datum *columnValues               = NULL ;
   bool *columnNulls                 = NULL ;
   Oid foreignTableId                = InvalidOid ;
   TupleDesc tupleDescriptor         = NULL ;
   Form_pg_attribute *attributesPtr  = NULL ;
   AttrNumber columnCount            = 0 ;
   AttrNumber columnId               = 0 ;
   HTAB *columnMappingHash           = NULL ;
   sdbCursorHandle hCursor           = SDB_INVALID_HANDLE ;
   List *columnList                  = NIL ;
   ForeignScanState *scanState       = NULL ;
   List *foreignPrivateList          = NIL ;
   ForeignScan *foreignScan          = NULL ;
   CHAR *relationName                = NULL ;
   INT32 executorFlags               = 0 ;
   MemoryContext oldContext          = CurrentMemoryContext ;
   MemoryContext tupleContext        = NULL ;
   sdbbson queryDocument ;
   SdbExecState *fdw_state            = NULL ;

   sdbbson_init( &queryDocument ) ;

   /* create columns in the relation */
   tupleDescriptor = RelationGetDescr( relation ) ;
   columnCount     = tupleDescriptor->natts ;
   attributesPtr   = tupleDescriptor->attrs ;

   for( columnId = 1 ; columnId <= columnCount ; ++columnId )
   {
      Var *column       = ( Var* )palloc0( sizeof( Var ) ) ;
      if( !column )
      {
         ereport( ERROR,( errcode( ERRCODE_FDW_OUT_OF_MEMORY ),
                            errmsg( "Unable to allocate Var memory" ),
                            errhint( "Make sure the memory pool or ulimit is "
                                      "properly configured" ) ) ) ;
         goto error ;
      }
      column->varattno  = columnId ;
      column->vartype   = attributesPtr[columnId-1]->atttypid ;
      column->vartypmod = attributesPtr[columnId-1]->atttypmod ;
      columnList        = lappend( columnList, column ) ;
   }

   foreignTableId = RelationGetRelid( relation ) ;
   fdw_state = palloc0( sizeof( SdbExecState ) ) ;
   initSdbExecState( fdw_state ) ;
   sdbGetSdbServerOptions( foreignTableId, fdw_state ) ;

   fdw_state->pgTableDesc = sdbGetPgTableDesc( foreignTableId ) ;
   if ( NULL == fdw_state->pgTableDesc )
   {
      sdbFreeScanState( fdw_state, true ) ;
      ereport( ERROR,( errcode( ERRCODE_FDW_OUT_OF_MEMORY ),
            errmsg( "Unable to allocate pgTableDesc" ),
            errhint( "Make sure the memory pool or ulimit is properly configured" ) ) ) ;
      goto error ;
   }

   /* create state structure */
   scanState = makeNode( ForeignScanState ) ;
   scanState->ss.ss_currentRelation = relation ;

   foreignPrivateList = serializeSdbExecState( fdw_state ) ;
   foreignPrivateList = list_make2( foreignPrivateList, columnList ) ;

   foreignScan              = makeNode( ForeignScan ) ;
   foreignScan->fdw_private = foreignPrivateList ;
   scanState->ss.ps.plan = ( Plan * )foreignScan ;
   SdbBeginForeignScan( scanState, executorFlags ) ;

   fdw_state = ( SdbExecState* )scanState->fdw_state ;
   rc = sdbQuery1( fdw_state->hCollection, &fdw_state->queryDocument, NULL,
                   NULL, NULL, 0, -1, FLG_QUERY_WITH_RETURNDATA,
                   &fdw_state->hCursor ) ;
   if ( rc )
   {
      ereport( ERROR, ( errcode ( ERRCODE_FDW_ERROR ),
               errmsg( "query collection failed:cs=%s,cl=%s,rc=%d",
                       fdw_state->sdbcs, fdw_state->sdbcl, rc ) ) ) ;
      sdbbson_dispose( &fdw_state->queryDocument ) ;
      goto error ;
   }

   hCursor = fdw_state->hCursor ;
   columnMappingHash = fdw_state->columnMappingHash ;

   tupleContext = AllocSetContextCreate( CurrentMemoryContext,
                                          "sdb_fdw temp context",
                                          ALLOCSET_DEFAULT_MINSIZE,
                                          ALLOCSET_DEFAULT_INITSIZE,
                                          ALLOCSET_DEFAULT_MAXSIZE ) ;
   /* prepare for sampling rows */
   randomState  = anl_init_selection_state( targetRowCount ) ;
   columnValues = ( Datum * )palloc0( columnCount * sizeof( Datum ) ) ;
   columnNulls  = ( bool* )palloc0( columnCount * sizeof( bool ) ) ;
   if( !columnValues || !columnNulls )
   {
      sdbFreeScanState( fdw_state, true ) ;
      ereport( ERROR,( errcode( ERRCODE_FDW_OUT_OF_MEMORY ),
                         errmsg( "Unable to allocate Var memory" ),
                         errhint( "Make sure the memory pool or ulimit is "
                                   "properly configured" ) ) ) ;
      goto error ;
   }
   while( TRUE )
   {
      sdbbson recordObj ;
      sdbbson_init( &recordObj ) ;
      /* check for any break or terminate events */
      vacuum_delay_point(  ) ;
      /* init all values for this row to null */
      memset( columnValues, 0, columnCount * sizeof( Datum ) ) ;
      memset( columnNulls, true, columnCount * sizeof( bool ) ) ;
      rc = sdbNext( hCursor, &recordObj ) ;
      if( rc )
      {
         sdbbson_destroy( &recordObj ) ;
         if( SDB_DMS_EOC != rc )
         {
            sdbFreeScanState( fdw_state, true ) ;
            /* if other error happened, let's report them */
            ereport( ERROR,( errcode( ERRCODE_FDW_ERROR ),
                               errmsg( "unable to fetch next record"
                                        ", rc = %d", rc ),
                               errhint( "Make sure collection exists on remote"
                                         " SequoiaDB database, and the server "
                                         "is still up and running" ) ) ) ;
            goto error ;
         }
         break ;
      }
      /* if we can read the next record */
      MemoryContextReset( tupleContext ) ;
      MemoryContextSwitchTo( tupleContext ) ;
      sdbFillTupleSlot( &recordObj, NULL, columnMappingHash,
                        fdw_state->isUseDecimal, columnValues, columnNulls ) ;
      MemoryContextSwitchTo( oldContext ) ;

      if( sampleRowCount < targetRowCount )
      {
         sampleRows[sampleRowCount++] = heap_form_tuple( tupleDescriptor,
                                                          columnValues,
                                                          columnNulls ) ;
      }
      else
      {
         if( rowCountToSkip < 0 )
         {
            rowCountToSkip = anl_get_next_S( rowCount, targetRowCount,
                                              &randomState ) ;
         }
         if( rowCountToSkip <= 0 )
         {
            /* process the row if we skipped enough records */
            INT32 rowIndex = ( INT32 )( targetRowCount * anl_random_fract(  ) ) ;
            heap_freetuple( sampleRows[rowIndex] ) ;
            sampleRows[rowIndex] = heap_form_tuple( tupleDescriptor,
                                                     columnValues,
                                                     columnNulls ) ;
         }
         rowCountToSkip -= 1 ;
      }
      rowCount += 1 ;
      sdbbson_destroy( &recordObj ) ;
   }

   sdbFreeScanState( fdw_state, true ) ;
done :
   MemoryContextDelete( tupleContext ) ;
   pfree( columnValues ) ;
   pfree( columnNulls ) ;
   sdbbson_destroy( &queryDocument ) ;

   /* get some result */
   relationName = RelationGetRelationName( relation ) ;
   ereport( errorLevel, ( errmsg( "\"%s\": collection contains %lld rows; "
                                   "%d rows in sample",
                                   relationName, rowCount, sampleRowCount ) ) ) ;
   ( *totalRowCount )= rowCount ;
   ( *totalDeadRowCount )= 0 ;
   return sampleRowCount ;
error :
   goto done ;
}

/* SdbAnalyzeForeignTable collects statistics for the given foreign table
 */
static bool SdbAnalyzeForeignTable (
      Relation relation,
      AcquireSampleRowsFunc *acquireSampleRowsFunc,
      BlockNumber *totalPageCount )
{
   INT32 rc                 = SDB_OK ;
   BlockNumber pageCount    = 0 ;
   INT32 attributeCount     = 0 ;
   INT32 *attributeWidths   = NULL ;
   Oid foreignTableId       = InvalidOid ;
   INT32 documentWidth      = 0 ;
   INT64 rowsCount          = 0 ;
   FLOAT64 foreignTableSize = 0 ;

   foreignTableId = RelationGetRelid( relation ) ;
   rc = sdbRowsCount( foreignTableId, &rowsCount ) ;
   if( rc )
   {
      goto error ;
   }
   if( rowsCount >= 0 )
   {
      attributeCount  = RelationGetNumberOfAttributes( relation ) ;
      attributeWidths = ( INT32* )palloc0( ( attributeCount + 1 )*sizeof( INT32 ) ) ;
      if( !attributeWidths )
      {
         ereport( ERROR,( errcode( ERRCODE_FDW_OUT_OF_MEMORY ),
                            errmsg( "Unable to allocate attr array memory" ),
                            errhint( "Make sure the memory pool or ulimit is "
                                      "properly configured" ) ) ) ;
         goto error ;
      }
      documentWidth = get_relation_data_width( foreignTableId,
                                                attributeWidths ) ;
      foreignTableSize = rowsCount * documentWidth ;
      pageCount = ( BlockNumber )rint( foreignTableSize / BLCKSZ ) ;
   }
   else
   {
      ereport( ERROR,( errmsg( "Unable to retreive rows count" ),
                         errhint( "Unable to collect stats about foreign "
                                   "table" ) ) ) ;
   }
done :
   ( *totalPageCount )= pageCount ;
   ( *acquireSampleRowsFunc )= sdbAcquireSampleRows ;
   return TRUE ;
error :
   goto done ;
}


static sdbCheckThreadInfo *sdbGetCheckThreadInfo()
{
   static sdbCheckThreadInfo info ;
   return &info ;
}

static void sdbStartInterruptCheck()
{
   int rc = 0 ;
   sdbCheckThreadInfo *threadInfo = sdbGetCheckThreadInfo() ;
   threadInfo->checkThreadID = 0 ;
   threadInfo->running       = 1 ;
   rc = pthread_create( &threadInfo->checkThreadID, NULL, thr_check_interrupt,
                        NULL ) ;
   if ( 0 != rc )
   {
      threadInfo->running = 0 ;
      elog( ERROR, "create interrrupt check thread failed:rc=%d", rc ) ;
   }

   atexit( sdbJoinInterruptThread ) ;
}

static void sdbJoinInterruptThread()
{
   sdbCheckThreadInfo *threadInfo = sdbGetCheckThreadInfo() ;
   threadInfo->running = 0 ;
   pthread_join( threadInfo->checkThreadID, NULL ) ;
}

static void *thr_check_interrupt( void * arg )
{
   sdbCheckThreadInfo *threadInfo = sdbGetCheckThreadInfo() ;
   while ( threadInfo->running )
   {
      if ( sdbIsInterrupt() )
      {
         threadInfo->running = 0 ;
         fprintf(stderr, "ERROR: aaaaaaaaaaaaaaaa\n");
         elog( ERROR, "QueryCancelPending interrupt received!" ) ;
         return NULL ;
      }

      pg_usleep(1000000); // 1 second
   }

   return NULL ;
}

#if PG_VERSION_NUM>90300

static INT32 SdbIsForeignRelUpdatable( Relation rel )
{
   return( 1 << CMD_INSERT )|( 1 << CMD_UPDATE )|
         ( 1 << CMD_DELETE ) ;
}

static void SdbAddForeignUpdateTargets( Query *parsetree, RangeTblEntry *target_rte,
      Relation target_relation )
{




}

 static List *SdbPlanForeignModify( PlannerInfo *root, ModifyTable *plan,
      Index resultRelation, INT32 subplan_index )
{
   RangeTblEntry *rte           = NULL ;
   Oid foreignTableId ;
   SdbExecState *fdw_state      = NULL ;
   List *listModify             = NIL ;

   if ( resultRelation < root->simple_rel_array_size
         && root->simple_rel_array[resultRelation] != NULL )
   {
      fdw_state = ( SdbExecState * )( root->simple_rel_array[resultRelation]->fdw_private ) ;
   }
   else
   {
      /* allocate new execution state */
      fdw_state = ( SdbExecState* )palloc( sizeof( SdbExecState ) ) ;
      if ( !fdw_state )
      {
         ereport( ERROR,( errcode( ERRCODE_FDW_OUT_OF_MEMORY ),
               errmsg( "Unable to allocate execution state memory" ),
               errhint( "Make sure the memory pool or ulimit is properly configured" ) ) ) ;

         return NULL ;
      }

      initSdbExecState( fdw_state ) ;
      rte = planner_rt_fetch( resultRelation, root ) ;
      foreignTableId = rte->relid ;
      sdbGetSdbServerOptions( foreignTableId, fdw_state ) ;

      fdw_state->pgTableDesc = sdbGetPgTableDesc( foreignTableId ) ;
      if ( NULL == fdw_state->pgTableDesc )
      {
         ereport( ERROR,( errcode( ERRCODE_FDW_OUT_OF_MEMORY ),
               errmsg( "Unable to allocate pgTableDesc" ),
               errhint( "Make sure the memory pool or ulimit is properly configured" ) ) ) ;

         return NULL ;
      }
   }

   listModify = serializeSdbExecState( fdw_state ) ;
   sdbbson_destroy( &fdw_state->queryDocument ) ;
   return listModify ;
}

void SdbBeginForeignModify( ModifyTableState *mtstate,
      ResultRelInfo *rinfo, List *fdw_private, int subplan_index, int eflags )
{

   SdbExecState *fdw_state = deserializeSdbExecState( fdw_private ) ;
   rinfo->ri_FdwState = fdw_state ;

   fdw_state->hConnection = sdbGetConnectionHandle(
                                       (const char **)fdw_state->sdbServerList,
                                       fdw_state->sdbServerNum,
                                       fdw_state->usr,
                                       fdw_state->passwd,
                                       fdw_state->preferenceInstance,
                                       fdw_state->transaction ) ;
   fdw_state->hCollection = sdbGetSdbCollection( fdw_state->hConnection,
      fdw_state->sdbcs, fdw_state->sdbcl ) ;


}

void sdb_slot_deform_tuple( TupleTableSlot *slot, int natts )
{
   HeapTuple tuple     = slot->tts_tuple;
   TupleDesc tupleDesc = slot->tts_tupleDescriptor;
   Datum *values       = slot->tts_values;
   bool *isnull        = slot->tts_isnull;
   HeapTupleHeader tup = tuple->t_data;
   bool hasnulls       = HeapTupleHasNulls(tuple);
   Form_pg_attribute *att = tupleDesc->attrs;
   int attnum;
   char *tp;           /* ptr to tuple data */
   long off;        /* offset in tuple data */
   bits8 *bp = tup->t_bits;      /* ptr to null bitmap in tuple */
   bool slow;       /* can we use/set attcacheoff? */

   /*
    * Check whether the first call for this tuple, and initialize or restore
    * loop state.
    */
   attnum = slot->tts_nvalid;
   if (attnum == 0)
   {
      /* Start from the first attribute */
      off = 0;
      slow = false;
   }
   else
   {
      /* Restore state from previous execution */
      off = slot->tts_off;
      slow = slot->tts_slow;
   }

   tp = (char *) tup + tup->t_hoff;

   for (; attnum < natts; attnum++)
   {
      Form_pg_attribute thisatt = att[attnum];

      if (hasnulls && att_isnull(attnum, bp))
      {
         values[attnum] = (Datum) 0;
         isnull[attnum] = true;
         slow = true;      /* can't use attcacheoff anymore */
         continue;
      }

      isnull[attnum] = false;

      if (!slow && thisatt->attcacheoff >= 0)
         off = thisatt->attcacheoff;
      else if (thisatt->attlen == -1)
      {
         /*
          * We can only cache the offset for a varlena attribute if the
          * offset is already suitably aligned, so that there would be no
          * pad bytes in any case: then the offset will be valid for either
          * an aligned or unaligned value.
          */
         if (!slow &&
            off == att_align_nominal(off, thisatt->attalign))
            thisatt->attcacheoff = off;
         else
         {
            off = att_align_pointer(off, thisatt->attalign, -1,
                              tp + off);
            slow = true;
         }
      }
      else
      {
         /* not varlena, so safe to use att_align_nominal */
         off = att_align_nominal(off, thisatt->attalign);

         if (!slow)
            thisatt->attcacheoff = off;
      }

      values[attnum] = fetchatt(thisatt, tp + off);

      off = att_addlength_pointer(off, thisatt->attlen, tp + off);

      if (thisatt->attlen <= 0)
         slow = true;      /* can't use attcacheoff anymore */
   }

   /*
    * Save state for next execution
    */
   slot->tts_nvalid = attnum;
   slot->tts_off = off;
   slot->tts_slow = slow;
}

TupleTableSlot *SdbExecForeignInsert( EState *estate, ResultRelInfo *rinfo,
      TupleTableSlot *slot, TupleTableSlot *planSlot )
{
   SdbExecState *fdw_state = ( SdbExecState * )rinfo->ri_FdwState ;
   int attnum = 0 ;
   PgTableDesc *tableDesc = NULL ;
   int rc = SDB_OK ;

   sdbbson insert ;
   sdbbson_init( &insert ) ;
   tableDesc = fdw_state->pgTableDesc ;
   if ( slot->tts_nvalid == 0 )
   {
      sdb_slot_deform_tuple( slot, tableDesc->ncols ) ;
   }

   for (  ; attnum < tableDesc->ncols ; attnum++ )
   {
      if ( slot->tts_isnull[attnum] )
      {
         continue ;
      }

      rc = sdbSetBsonValue( &insert, tableDesc->cols[attnum].pgname,
                            slot->tts_values[attnum],
                            tableDesc->cols[attnum].pgtype,
                            tableDesc->cols[attnum].pgtypmod,
                            fdw_state->isUseDecimal ) ;
      if ( SDB_OK != rc )
      {
         ereport ( WARNING, ( errcode( ERRCODE_FDW_INVALID_DATA_TYPE ),
                   errmsg( "convert value failed:key=%s",
                   tableDesc->cols[attnum].pgname ) ) ) ;
         sdbbson_destroy( &insert ) ;
         return NULL ;
      }
   }

   rc = sdbbson_finish( &insert ) ;
   if ( rc != SDB_OK )
   {
      ereport( WARNING, ( errcode( ERRCODE_FDW_ERROR ),
            errmsg( "finish bson failed:rc = %d", rc ),
            errhint( "Make sure the data type is all right" ) ) ) ;

      sdbbson_destroy( &insert ) ;
      return NULL ;
   }

   rc = sdbInsert( fdw_state->hCollection, &insert ) ;
   if ( rc != SDB_OK )
   {
      ereport( ERROR, ( errcode( ERRCODE_FDW_ERROR ),
            errmsg( "sdbInsert failed:rc = %d", rc ),
            errhint( "Make sure the data type is all right" ) ) ) ;

      sdbbson_destroy( &insert ) ;
      return NULL ;
   }

   sdbbson_destroy( &insert ) ;

   /* store the virtual tuple */
   return slot ;
}

TupleTableSlot *SdbExecForeignDelete( EState *estate, ResultRelInfo *rinfo,
      TupleTableSlot *slot, TupleTableSlot *planSlot )
{
   int i   = 0 ;
   sdbbson sdbbsonCondition ;
   sdbbson *original ;
   int rc = SDB_OK ;
   SdbExecState *fdw_state = ( SdbExecState * )rinfo->ri_FdwState ;

   sdbbson_init( &sdbbsonCondition ) ;
   original = sdbGetRecordPointer( fdw_state->bson_record_addr ) ;
   for ( i = 0 ; i < fdw_state->key_num ; i++ )
   {
      sdbbson_iterator ite ;
      sdbbson_find( &ite, original, fdw_state->key_name[i] ) ;
      sdbbson_append_element( &sdbbsonCondition, NULL, &ite ) ;
   }
   sdbbson_finish( &sdbbsonCondition ) ;

   rc = sdbDelete( fdw_state->hCollection, &sdbbsonCondition, NULL ) ;
   if ( rc != SDB_OK )
   {
      sdbbson_destroy( &sdbbsonCondition ) ;
      ereport( ERROR, ( errcode( ERRCODE_FDW_ERROR ),
            errmsg( "sdbDelete failed:rc = %d", rc ),
            errhint( "Make sure the data type is all right" ) ) ) ;
      return NULL ;
   }

   sdbbson_destroy( &sdbbsonCondition ) ;

   ExecClearTuple( slot ) ;
   ExecStoreVirtualTuple( slot ) ;

   return slot ;
}

TupleTableSlot *SdbExecForeignUpdate( EState *estate, ResultRelInfo *rinfo,
      TupleTableSlot *slot, TupleTableSlot *planSlot )
{
   int i   = 0 ;
   sdbbson sdbbsonCondition ;
   sdbbson sdbbsonValues ;
   sdbbson sdbbsonTempValue ;
   sdbbson *original = NULL ;
   Datum datum ;
   int rc = SDB_OK ;
   bool isnull ;
   SdbExecState *fdw_state = ( SdbExecState * )rinfo->ri_FdwState ;

   sdbbson_init( &sdbbsonTempValue ) ;
   for ( i = 0 ; i < fdw_state->pgTableDesc->ncols ; i++ )
   {
      datum = slot_getattr( slot, fdw_state->pgTableDesc->cols[i].pgattnum, &isnull ) ;
      if ( !isnull )
      {
         rc = sdbSetBsonValue( &sdbbsonTempValue,
                               fdw_state->pgTableDesc->cols[i].pgname,
                               datum, fdw_state->pgTableDesc->cols[i].pgtype,
                               fdw_state->pgTableDesc->cols[i].pgtypmod,
                               fdw_state->isUseDecimal ) ;
         if ( SDB_OK != rc )
         {
            ereport ( WARNING, ( errcode( ERRCODE_FDW_INVALID_DATA_TYPE ),
                      errmsg( "convert value failed:key=%s",
                      fdw_state->pgTableDesc->cols[i].pgname ) ) ) ;
            sdbbson_destroy( &sdbbsonTempValue ) ;
            return NULL ;
         }
      }
   }
   sdbbson_finish( &sdbbsonTempValue ) ;

   sdbbson_init( &sdbbsonCondition ) ;
   original = sdbGetRecordPointer( fdw_state->bson_record_addr ) ;
   for ( i = 0 ; i < fdw_state->key_num ; i++ )
   {
      sdbbson_iterator ite ;
      sdbbson_find( &ite, original, fdw_state->key_name[i] ) ;
      sdbbson_append_element( &sdbbsonCondition, NULL, &ite ) ;
   }
   sdbbson_finish( &sdbbsonCondition ) ;

   if ( sdbIsShardingKeyChanged( fdw_state, original, &sdbbsonTempValue ) )
   {
      sdbbson_destroy( &sdbbsonCondition ) ;
      sdbbson_destroy( &sdbbsonTempValue ) ;

      ereport( ERROR, ( errcode( ERRCODE_FDW_ERROR ),
            errmsg( "update failed due to shardingkey is changed" ) ) ) ;

      return NULL ;
   }

   sdbbson_init( &sdbbsonValues ) ;
   sdbbson_append_sdbbson( &sdbbsonValues, "$set", &sdbbsonTempValue ) ;
   sdbbson_finish( &sdbbsonValues ) ;

   rc = sdbUpdate( fdw_state->hCollection, &sdbbsonValues, &sdbbsonCondition, NULL ) ;
   if ( rc != SDB_OK )
   {
      sdbbson_destroy( &sdbbsonCondition ) ;
      sdbbson_destroy( &sdbbsonTempValue ) ;
      sdbbson_destroy( &sdbbsonValues ) ;
      ereport( WARNING, ( errcode( ERRCODE_FDW_ERROR ),
            errmsg( "sdbUpdate failed:rc = %d", rc ),
            errhint( "Make sure the data type is all right" ) ) ) ;
      return NULL ;
   }

   sdbbson_destroy( &sdbbsonCondition ) ;
   sdbbson_destroy( &sdbbsonTempValue ) ;
   sdbbson_destroy( &sdbbsonValues ) ;


   return slot ;
}


void SdbEndForeignModify( EState *estate, ResultRelInfo *rinfo )
{
   SdbExecState *fdw_state = ( SdbExecState * )rinfo->ri_FdwState ;
   if ( fdw_state )
   {
      sdbFreeScanStateInModifyEnd( fdw_state ) ;
   }
}

void SdbExplainForeignModify( ModifyTableState *mtstate, ResultRelInfo *rinfo,
      List *fdw_private, int subplan_index, struct ExplainState *es )
{
}

#endif

/* Transaction related */

static void SdbFdwXactCallback( XactEvent event, void *arg )
{
   INT32 count             = 0 ;
   SdbConnectionPool *pool = sdbGetConnectionPool(  ) ;
   for( count = 0 ; count < pool->numConnections ; ++count )
   {
      SdbConnection *conn = &pool->connList[count] ;
      /* if there's no transaction started on this connection, let's continue */
      if( 0 == conn->transLevel )
         continue ;
      /* attempt to commit/abort the transaction, ignore return code */
      switch( event )
      {
      case XACT_EVENT_COMMIT :
         if ( 1 == conn->isTransactionOn && conn->transLevel > 0 )
         {
            elog( DEBUG1, "trans commit[%s]", conn->connName ) ;
            sdbTransactionCommit( conn->hConnection ) ;
            conn->transLevel = 0 ;
         }

         break ;
      case XACT_EVENT_ABORT :
         if ( 1 == conn->isTransactionOn && conn->transLevel > 0 )
         {
            elog( DEBUG1, "trans rollback[%s]", conn->connName ) ;
            sdbTransactionRollback( conn->hConnection ) ;
            conn->transLevel = 0 ;
         }
         break ;
      default :
         break ;
      }
   }
}

SdbCLStatistics *SdbGetCLStatFromCache( Oid foreignTableId )
{
   INT32 rc = SDB_OK ;
   SdbStatisticsCache *cache = NULL ;
   SdbCLStatistics *statInCache = NULL ;
   bool handleFound = false ;
   cache = SdbGetStatisticsCache() ;

   statInCache = ( SdbCLStatistics * )
                 hash_search( cache->ht,
                 &foreignTableId,
                 HASH_ENTER,
                 &handleFound ) ;
   if ( NULL == statInCache )
   {
      goto error ;
   }

   elog( DEBUG1, "foreign table:%d, handle found:%d", foreignTableId, handleFound ) ;

   if ( !handleFound )
   {
      rc = SdbInitCLStatistics( statInCache ) ;
      if ( SDB_OK != rc )
      {
         SdbCLStatistics *toRemove = ( SdbCLStatistics * )
                                      hash_search( cache->ht,
                                      &foreignTableId,
                                      HASH_REMOVE,
                                      &handleFound ) ;
         if ( NULL != toRemove )
         {
            SdbDestroyCLStatistics( toRemove,  true ) ;
         }
         goto error ;
      }
   }
done:
   return statInCache ;
error:
   statInCache = NULL ;
   ereport( ERROR,( errcode( ERRCODE_FDW_ERROR ),
                       errmsg( "cannot get stat of cl from cache" ),
                       errhint( "foreign table id:%d", foreignTableId ) ) ) ;
   goto done ;
}











SdbStatisticsCache *SdbGetStatisticsCache()
{
   static SdbStatisticsCache cache ;
   return &cache ;
}

static void SdbInitCLStatisticsCache( SdbStatisticsCache *cache )
{
   HASHCTL hashInfo ;
   const long HASH_TB_SIZE = 2048 ;
   memset( &hashInfo, 0, sizeof( hashInfo ) ) ;
   hashInfo.keysize = sizeof( Oid ) ;
   hashInfo.entrysize = sizeof( SdbCLStatistics ) ;
   hashInfo.hash = oid_hash ;

   if ( NULL == cache || NULL != cache->ht )
   {
      ereport( WARNING, ( errcode( ERRCODE_FDW_ERROR ),
               errmsg( "the statistics cache is not valid" ) ) ) ;
      goto error ;
   }

   cache->ht = hash_create( "CL Statistics Hash",
                            HASH_TB_SIZE,
                            &hashInfo,
                            ( HASH_ELEM | HASH_FUNCTION ) ) ;
   if ( NULL == cache->ht )
   {
      ereport( WARNING,( errcode( ERRCODE_FDW_ERROR ),
                       errmsg( "failed to init statistics cache" ) ) );
      goto error ;
   }
done:
   return ;
error:
   SdbFiniCLStatisticsCache( cache ) ;
   goto done ;
}

static void SdbFiniCLStatisticsCache( SdbStatisticsCache *cache )
{
   if ( NULL != cache )
   {
      if ( NULL != cache->ht )
      {
         HASH_SEQ_STATUS status ;
         hash_seq_init( &status, cache->ht ) ;

         while ( true )
         {
            SdbCLStatistics *clStat = ( SdbCLStatistics * )
                                      hash_seq_search( &status ) ;
            if ( NULL == clStat )
            {
               break ;
            }

            SdbDestroyCLStatistics( clStat, true ) ;
         }
         hash_destroy( cache->ht ) ;
         cache->ht = NULL ;
      }
   }
   return ;
}

static INT32 SdbInitCLStatistics( SdbCLStatistics *clStat )
{
   INT32 rc = SDB_OK ;
   SdbInputOptions options ;
   sdbConnectionHandle conn = SDB_INVALID_HANDLE ;
   Oid oid = clStat->tableID ;

   memset( clStat, 0, sizeof( SdbCLStatistics ) ) ;

   clStat->indexNum = -1;

   sdbGetOptions( oid, &options ) ;
   clStat->tableID = oid ;
   rc = sdbRowsCountFromSdb( oid, &(clStat->recordCount) ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = sdbGetShardingKeyInfo( &options, clStat ) ;
   if ( SDB_OK != rc && rc != SDB_RTN_COORD_ONLY )
   {
      goto error ;
   }
   else
   {
      rc = SDB_OK ;
   }

   conn = sdbGetConnectionHandle( (const char **)options.serviceList,
                                   options.serviceNum, options.user,
                                   options.password,
                                   options.preference_instance,
                                   options.transaction ) ;
   if ( SDB_INVALID_HANDLE == conn )
   {
      rc = SDB_NETWORK ;
      goto error ;
   }

   clStat->clHandle = sdbGetSdbCollection( conn,
                                           options.collectionspace,
                                           options.collection ) ;
   if ( SDB_INVALID_HANDLE == clStat->clHandle )
   {
      rc = SDB_SYS ;
      goto error ;
   }
done:
   return rc ;
error:
   SdbDestroyCLStatistics( clStat, false ) ;
   goto done ;
}

static void SdbDestroyCLStatistics( SdbCLStatistics *clStat, bool freeObj )
{
   if ( NULL != clStat )
   {
      sdbReleaseCollection( clStat->clHandle ) ;
      memset( clStat, 0, sizeof( SdbCLStatistics ) ) ;
      if ( freeObj )
      {
         free( clStat ) ;
      }
   }
   return ;
}

void _PG_init (  )
{
   sdbInitConnectionPool (  ) ;
   SdbInitCLStatisticsCache( SdbGetStatisticsCache() ) ;
   /* we may lose the the pointer of record if it is temporary, so we must
      keep it in global */
   SdbInitRecordCache() ;
   RegisterXactCallback( SdbFdwXactCallback, NULL ) ;
}

void _PG_fini (  )
{
   SdbFiniCLStatisticsCache( SdbGetStatisticsCache() ) ;
   sdbUninitConnectionPool (  ) ;
   SdbFiniRecordCache() ;
}
