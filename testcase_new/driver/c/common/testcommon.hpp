#ifndef TESTCOMMON_HPP__
#define TESTCOMMON_HPP__

#if defined(__linux__)
#include <sys/syscall.h>
#elif defined(_AIX)
#include <pthread.h>
#endif

#include "client.h"
#include <stdio.h>
#include <vector>
#include <string>
#include <stdarg.h>

using namespace std ;

// tdd begin

#define SERVER1               "11800" // for catalog
#define SERVER2               "20100" // for data
#define USER1                 "sequoiadb"
#define PASSWD1               "sequoiadb"

#define COLLECTION_SPACE_NAME "testfoo"
#define COLLECTION_NAME       "testbar"
#define COLLECTION_NAME1      "testbar1"
#define COLLECTION_FULL_NAME  "testfoo.testbar"
#define INDEX_NAME            "testIndex"
#define SOURCEGROUP           "group1"
#define TARGETGROUP           "group2"
#define NUM_RECORD            5

// for rg
#define GROUPNAME1            "testgroup1"
#define GROUPNAME2            "testgroup2"
#define GROUPNAME3            "testgroup3"

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
#define MAX_NAME_SIZE         127

#define CHECK_MSG(fmt, args ...) printf("%s[%d]:" fmt,__FILE__,__LINE__,##args)

// tdd end

#define CHECK_RC( expRc, actRc, fmt, ... ) \
do { \
   if( expRc != actRc ) \
   { \
      printMsg( fmt, ##__VA_ARGS__ ) ; \
      cout << endl ; \
      goto error ; \
   } \
} while( 0 ) ;

#define SDB_TEST_ERROR -10000

SDB_EXTERN_C_START

void printMsg( const CHAR* fmt, ... ) ;

// create normal cs cl, no option
INT32 createNormalCsCl( sdbConnectionHandle db, sdbCSHandle* cs, sdbCollectionHandle* cl,
				            const CHAR* csName, const CHAR* clName ) ;

BOOLEAN isStandalone( sdbConnectionHandle db ) ;

// get local hostname
INT32 getLocalHost( CHAR hostName[], INT32 len ) ;

// get database hostname
INT32 getDBHost( sdbConnectionHandle db, CHAR hostName[], INT32 len ) ;

// get idle port between RSRVPORTBEGIN and RSRVPORTEND
void getIdlePort( CHAR* port ) ;

// get all data groups
INT32 getGroups( sdbConnectionHandle db, vector<string>& groups ) ;

// get all nodes in group
INT32 getGroupNodes( sdbConnectionHandle db, const CHAR* rgName, vector<string>& nodes ) ;

// create lob and write buff
INT32 createLob( sdbCollectionHandle cl, bson_oid_t oid, const CHAR* buff, INT32 len ) ;

// write buff to lob
INT32 writeLob( sdbCollectionHandle cl, bson_oid_t oid, const CHAR* buff, INT32 len ) ;

// read lob, len: length want to read, read: actual read len
INT32 readLob( sdbCollectionHandle cl, bson_oid_t oid, CHAR* buff, UINT32 len, UINT32* read ) ;

INT32 getLobSize( sdbCollectionHandle cl, bson_oid_t oid, SINT64* size ) ;

BOOLEAN isOidEqual( bson_oid_t oid1, bson_oid_t oid2 ) ;

// tdd begin

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

// tdd end

SDB_EXTERN_C_END

#endif
