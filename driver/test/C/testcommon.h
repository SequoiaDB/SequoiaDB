/******************************************************************************
 *
 * Name: common.h * Description: Common functions declaration for sample programs
 *
 ******************************************************************************/
#ifndef COMMON_H__
#define COMMON_H__
#if defined(__linux__)
#include <sys/syscall.h>
#elif defined(_AIX)
#include <pthread.h>
#endif
#include "client.h"

//#define HOST                  "localhost"
//#define SERVER                "11810" // for coord
#define HOST                  "localhost"
#define SERVER                "50000" // for coord
#define SERVER1               "30000" // for catalog
#define SERVER2               "20000" // for data
#define USER                  ""
#define PASSWD                ""
#define USER1                 "sequoiadb"
#define PASSWD1               "sequoiadb"
#define COLLECTION_SPACE_NAME "testfoo"
#define COLLECTION_NAME       "testbar"
#define COLLECTION_NAME1      "testbar1"
#define COLLECTION_FULL_NAME  "testfoo.testbar"
#define PRIMARY_COLLECTION_NAME  "primaryCL"
#define SUB_A_COLLECTION_FULL_NAME  "testfoo.subACL"
#define SUB_B_COLLECTION_FULL_NAME  "testfoo.subBCL"
#define INDEX_NAME            "testIndex"
#define SOURCEGROUP           "group1"
#define TARGETGROUP           "group2"
#define NUM_RECORD            5


// for rg
#define GROUPNAME1            "db1"
#define GROUPNAME2            "db2"

#define DATAPATH1             "/opt/sequoiadb/database/test/data1"
#define DATAPATH2             "/opt/sequoiadb/database/test/data2"
#define DATAPATH3             "/opt/sequoiadb/database/test/data3"


#define _DATAPATH1            "/home/users/tanzhaobo/sequoiadb/path/data1"
#define _DATAPATH2            "/home/users/tanzhaobo/sequoiadb/path/data2"
#define _DATAPATH3            "/home/users/tanzhaobo/sequoiadb/path/data3"

#define SERVER_NAME1          "58000"
#define SERVER_NAME2          "58100"
#define SERVER_NAME3          "58200"

#define NAME_LEN              255

#define CHECK_MSG(fmt, args ...) printf("%s[%d]:"fmt,__FILE__,__LINE__,##args)

SDB_EXTERN_C_START
/* display syntax error */
void displaySyntax ( CHAR *pCommand );

/* create record list */
void createRecordList ( bson *objlist, INT32 listSize ) ;

/* connect to a given database */
INT32 connectTo ( const CHAR *pHostName,
                  const CHAR *pServiceName,
                  const CHAR *pUsr,
                  const CHAR *pPasswd,
                  sdbConnectionHandle *connection ) ;

/* get collection space, if the collection does not exist it will try to create
 * one */
INT32 getCollectionSpace ( sdbConnectionHandle connection,
                           const CHAR *pCSName,
                           sdbCSHandle *collectionSpace ) ;

/* drop the collection space */
INT32 dropCollectionSpace ( sdbConnectionHandle cHandle,
                            const CHAR *pCollectionSpaceName ) ;

/* get a collection, if the collection does not exist, it will try to create
 * one */
INT32 getCollection ( sdbConnectionHandle connection,
                      const CHAR *pCollectionFullName,
                      sdbCollectionHandle *collection ) ;

/* insert record into collection */
INT32 insertRecord ( sdbCollectionHandle collection,
                     bson *obj ) ;

INT32 bulkInsertRecords ( sdbCollectionHandle collection,
                          SINT32 flags, bson **obj, SINT32 num ) ;

CHAR *loadTag ( CHAR *pString ) ;

BOOLEAN isComment ( CHAR *pString ) ;

BOOLEAN loadJSON ( CHAR *pString, bson *obj ) ;

INT32 query ( sdbCollectionHandle cHandle,
              bson *condition,
              bson *select,
              bson *orderBy,
              bson *hint,
              INT64 numToSkip,
              INT64 numToReturn,
              sdbCursorHandle *handle ) ;

INT32 getIndexes ( sdbCollectionHandle collection,
                   const CHAR *pIndexName,
                   sdbCursorHandle *handle ) ;

INT32 deleteRecords ( sdbCollectionHandle collection,
                      bson *cond,
                      bson *hint ) ;

INT32 createIndex ( sdbCollectionHandle collection,
                    bson *indexdef,
                    const CHAR *pIndexName,
                    BOOLEAN isUnique,
                    BOOLEAN isEnforced ) ;

INT32 fetchRecords ( sdbCollectionHandle collection,
                     bson *condition,
                     bson *selector,
                     bson *orderBy,
                     bson *hint,
                     INT64 skip,
                     INT64 numReturn ) ;

/* initialize the environment */
INT32 initEnv( const CHAR* host, const CHAR* server,
               const CHAR* user, const CHAR* passwd ) ;
/* inset record into database */
void insertRecords( sdbCollectionHandle cl, INT32 num ) ;
void insertRecords1( sdbCollectionHandle cl, INT32 num ) ;

/* build up the cs and cl*/
void buildCSCL( const CHAR* host, const CHAR* server,
                const CHAR* user, const CHAR* passwd ) ;

/* create record list */
void createRecordList1 ( bson *objlist, INT32 listSize ) ;

/* create English record */
void createEnglishRecord ( bson *obj ) ;

/* create Chinese record */
void createChineseRecord ( bson *obj ) ;

void createNameList ( bson *objlist, INT32 listSize ) ;

void displayRecord( sdbCursorHandle *cursor ) ;

/* gen records */
char *stringGen ( char *pString, int len ) ;
void getTime ( bson_timestamp_t *t ) ;
void recordAppendHistory ( bson *obj, int numEle ) ;
void recordGen ( bson *obj, char *pMacBuffer, long serialNum ) ;
int insertToDB ( sdbConnectionHandle *sdb, const char *clFullName, long numToInsert ) ;
int genRecord ( sdbConnectionHandle *sdb, const char *clFullName, long num ) ;

/* get the acount of record in cursor */
long getRecordNum ( sdbCursorHandle cursor ) ;

/* get name have pid, add by xiaojun */
void getUniqueName( const CHAR *modName, CHAR *pBuffer ) ;

BOOLEAN isCluster( sdbConnectionHandle db ) ;

INT32 isTranOn( sdbConnectionHandle db, BOOLEAN *flag ) ;

INT32 gettid() ;

INT32 bson_compare(const CHAR *pStr, const bson *b) ;

CHAR* allocMemory( INT32 size ) ;

void freeMemory( CHAR *p ) ;

SDB_EXTERN_C_END

#endif
