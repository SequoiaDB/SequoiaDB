#ifndef TESTCOMMON_HPP__
#define TESTCOMMON_HPP__
#include <string>
#include "client.hpp"

#define COLLECTION_SPACE_NAME  "testfoo"
#define COLLECTION_NAME        "testbar"
#define COLLECTION_SPLIT       "split"
#define COLLECTION_FULL_NAME   "testfoo.testbar"
#define NOT_EXIST_CL_FULL_NAME "testfoo1.testbar1"
#define NOT_EXIST_CS_NAME      "testfoo1"
#define NOT_EXIST_CL_NAME      "testbar1"
//#define RGNAME                ""


#define INDEX_NAME            "testIndex"

#define GROUPNAME             "db1"
#define GROUPNAME1            "testgroup1"
#define GROUPNAME2            "testgroup2"
#define GROUPNAME3            "testgroup3"
#define BACKUPGROUPNAME GROUPNAME

#define DATAPATH1           "/opt/sequoiadb/database/test/node1"
#define DATAPATH2           "/opt/sequoiadb/database/test/node2"
#define DATAPATH3           "/opt/sequoiadb/database/test/node3"
#define BACKUPPATH          "/opt/sequoiadb/database/test/backup"


#define _DATAPATH1          "/home/users/tanzhaobo/data/node1"
#define _DATAPATH2          "/home/users/tanzhaobo/data/node2"
#define _DATAPATH3          "/home/users/tanzhaobo/data/node3"

#define HOST                "localhost"
#define SERVER              "50000"
#define SERVER1             "58000"
#define SERVER2             "58100"
#define SERVER3             "58200"
#define USER                ""
#define PASSWD              ""

// for data source
#define DATASOURCENAME        "dataSourceC++Test"
// A coord address of another cluster
#define DATASOURCEADDRESS     "sdbserver:11810"
// Multiple coord addresses of other clusters, spearated by ','
#define DATASOURCEURLS        "sdbserver:11910,sdbserver:11920"

// local coord node svcName for connection pool
#define CONNPOOL_LOCAL_SERVER1   "11810"
#define CONNPOOL_LOCAL_SERVER2   "50000"

#define BACKUPNAME          "backup_cpp_test"
#define CHECK_MSG(fmt, args ...) printf("%s[%d]:"fmt,__FILE__,__LINE__,##args) ;


using namespace sdbclient ;
using namespace std;
using namespace bson ;


/* get name have pid, add by xiaojun */
void getUniqueName( const CHAR *modName, CHAR *getName, INT32 len ) ;


/* connect to a given database */
INT32 connectTo ( const CHAR *pHostName,
                  const CHAR *pServiceName,
                  const CHAR *pUser,
                  const CHAR *pPasswd,
                  sdb &connection ) ;

/* get cs and cl, if not exist it will try to create one */
INT32 getCollectionSpace ( sdb &connection,
                           const CHAR *pCSName,
                           sdbCollectionSpace &collectionSpace ) ;
INT32 getCollection ( sdbCollectionSpace &cs,
                      const CHAR *pCollectionName,
                      sdbCollection &cl ) ;

/* get a collection, if the collection does not exist, it will try to create one */
INT32 getCollection1 ( sdb &connection,
                      const CHAR *pCollectionFullName,
                      sdbCollection &collection ) ;

/* insert record into collection */
INT32 insertRecord ( sdbCollection &collection,
                     BSONObj &obj ) ;

CHAR *loadTag ( CHAR *pString ) ;

BOOLEAN isComment ( CHAR *pString ) ;

BOOLEAN loadJSON ( CHAR *pString, BSONObj &obj ) ;

INT32 getIndexes ( sdbCollection &collection,
                   const CHAR *pIndexName,
                   sdbCursor &handle ) ;

INT32 deleteRecords ( sdbCollection &collection,
                      BSONObj &cond,
                      BSONObj &hint ) ;

INT32 createIndex ( sdbCollection &collection,
                    BSONObj &indexdef,
                    const CHAR *pIndexName,
                    BOOLEAN isUnique,
                    BOOLEAN isEnforced ) ;

INT32 fetchRecords ( sdbCollection &collection,
                     BSONObj &condition,
                     BSONObj &selector,
                     BSONObj &orderBy,
                     BSONObj &hint,
                     INT64 skip,
                     INT64 numReturn ) ;

string toJson( const BSONObj &b ) ;

/* initialize environment */
int initEnv( void ) ;

/* create En and Cn record */
void createEnglishRecord ( BSONObj &obj ) ;
void createChineseRecord ( BSONObj &obj ) ;

/* create name and record list */
void createNameList ( vector<BSONObj> &objlist, INT32 listSize ) ;
void createRecordList ( vector<BSONObj> &objlist, INT32 listSize ) ;

/* insert some record into database */
INT32 insertRecords ( sdbCollection &cl, SINT64 num ) ;

/* gen records and insert into database */
CHAR *stringGen ( CHAR *pString, INT32 len ) ;
void recordAppendHistory ( BSONObj &obj, INT32 numEle ) ;
void recordGen ( BSONObj &obj, CHAR *pMacBuffer, SINT32 serialNum ) ;
INT32 insertToDB ( sdbCollection &cl, SINT64 numToInsert ) ;
INT32 genRecord ( sdbCollection &cl, SINT64 num ) ;

/* get the acount of record in cursor */
SINT64 getRecordNum ( sdbCursor &cursor ) ;

/* display record */
void displayRecord( sdbCursor &cursor ) ;

INT32 delete_space( string &dest, const CHAR *src ) ;


/******************************************************
 * The follow functions are use in cluster environment
 ******************************************************/

INT32 addGroup ( sdb &connection, const CHAR *newGroupName, sdbReplicaGroup &rg ) ;

INT32 getHostName ( CHAR *host, INT32 len ) ;

void getDataPath ( CHAR *buffer, INT32 len, const CHAR *dp1, const CHAR *dp2 ) ;

/* get name have pid, add by xiaojun */
//void getUniqueName( const CHAR *modName, CHAR getName[] ) ;

BOOLEAN isCluster( sdb &db ) ;

CHAR* allocMemory( INT32 size ) ;
void freeMemory( CHAR *p ) ;

#endif
