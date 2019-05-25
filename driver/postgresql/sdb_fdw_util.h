#ifndef SDB_FDW_UTIL_H__
#define SDB_FDW_UTIL_H__

#include "client.h"
#include "fmgr.h"
#include "catalog/pg_foreign_server.h"
#include "catalog/pg_foreign_table.h"
#include "nodes/relation.h"
#include "sdb_fdw.h"


typedef enum
{
   SDB_VAR_VAR = 0,
   SDB_PARAM_VAR ,
   SDB_VAR_CONST ,
   SDB_UNSUPPORT_ARG_TYPE
} EnumSdbArgType;

typedef struct
{
   bool     nonempty;      /* True if lists are not all empty */
   /* Lists of RestrictInfos, one per index column */
   List     *indexclauses[INDEX_MAX_KEYS];
} sdbIndexClauseSet;

void sdbGetIndexEqclause( PlannerInfo *root, RelOptInfo *baserel, Oid tableID,
                          sdbIndexInfo *indexInfo,
                          sdbIndexClauseSet *clauseset ) ;

void sdbMatchJoinClausesToIndex( PlannerInfo *root, RelOptInfo *rel,
                                 Oid tableID, sdbIndexInfo *index,
                                 sdbIndexClauseSet *clauseset ) ;

void sdbMatchClauseToIndex( RelOptInfo *rel, Oid tableID, sdbIndexInfo *index,
                            RestrictInfo *rinfo,
                            sdbIndexClauseSet *clauseset ) ;

bool sdbMatchClauseToIndexcol( RelOptInfo *rel, Oid tableID,
                               sdbIndexInfo *index, int indexcol,
                               RestrictInfo *rinfo ) ;


int sdbGetIndexInfos( SdbExecState *sdbState, sdbIndexInfo *indexInfo,
                      int maxNum, int *indexNum ) ;


sdbConnectionHandle sdbGetConnectionHandle( const char **serverList,
                                            int serverNum,
                                            const char *usr,
                                            const char *passwd,
                                            const char *preference_instance,
                                            const char *transaction ) ;

sdbCollectionHandle sdbGetSdbCollection( sdbConnectionHandle connectionHandle,
      const char *sdbcs, const char *sdbcl ) ;


SdbConnectionPool *sdbGetConnectionPool() ;

int sdbSetConnectionPreference( sdbConnectionHandle hConnection,
                                const CHAR *preference_instance ) ;

BOOLEAN sdbIsInterrupt() ;

void sdbReleaseConnectionFromPool(int index) ;

IndexPath *sdb_build_index_paths(PlannerInfo *root, RelOptInfo *rel,
              sdbIndexInfo *sdbIndex, sdbIndexClauseSet *clauses,
              SdbExecState *fdw_state);

IndexPath *sdb_create_index_path(PlannerInfo *root, RelOptInfo *rel,
                                 IndexOptInfo *index, List *indexclauses,
                                 List *indexclausecols, List *indexorderbys,
                                 List *indexorderbycols, List *pathkeys,
                                 ScanDirection indexscandir, bool indexonly,
                                 Relids required_outer, double loop_count,
                                 SdbExecState *fdw_state);

EnumSdbArgType getArgumentType(List *arguments);

int sdbGenerateRescanCondition(SdbExecState *fdw_state, PlanState *planState,
                               sdbbson *rescanCondition);

void sdbPrintBson( sdbbson *bson, int log_level, const char *label ) ;

void debugClauseInfo( PlannerInfo *root, RelOptInfo *baserel, Oid tableID ) ;


/* record cache */
typedef struct
{
   sdbbson *record ;
   BOOLEAN isUsed ;
} SdbRecordItem ;


#define SDB_MAX_RECORD_SIZE 100
typedef struct
{
   INT32 size ;
   INT32 usedCount ;
   SdbRecordItem recordArray[ SDB_MAX_RECORD_SIZE ] ;
} SdbRecordCache ;

void SdbInitRecordCache() ;
void SdbFiniRecordCache() ;

SdbRecordCache *SdbGetRecordCache() ;
sdbbson *SdbAllocRecord( SdbRecordCache *recordCache, UINT64 *recordID ) ;
sdbbson *SdbGetRecord( SdbRecordCache *recordCache, UINT64 recordID ) ;
void SdbReleaseRecord( SdbRecordCache *recordCache, UINT64 recordID ) ;


#ifdef SDB_USE_OWN_POSTGRES
void sdbuseownpostgres() ;
#endif /* SDB_USE_OWN_POSTGRES */



#endif /*SDB_FDW_UTIL_H__*/


