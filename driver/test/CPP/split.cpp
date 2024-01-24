#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include "deleteFile.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;

#define TARGETRGNAME "targetRGGroupName"

/*
 * connect info
 */
class common
{
public :
   static const CHAR *host ;
   static const CHAR *port ;
   static const CHAR *user ;
   static const CHAR *passwd ;
} ;
const CHAR * common::host = HOST ;
const CHAR * common::port = SERVER ;
const CHAR * common::user = USER ;
const CHAR * common::passwd = PASSWD ;

/*
 * set global environment
 */
class splitEnvironment : public testing::Environment
{
public :
   virtual void SetUp ()
   {
      cout << "**************in SetUp()" << endl ;
   }
   virtual void TearDown ()
   {
      cout << "in TearDown()***********" << endl ;
   }
/*
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbShard rg ;
   sdbNode node ;
*/
} ;

splitEnvironment *split_env ;

/*
 * set split util function
 */
class split
{
public :
   split ()
   {
      initialize () ;
   }

   ~split ()
   {
      db.disconnect () ;
   }

   const CHAR* getHost ()
   {
      getHostName ( hostName, arrayLen ) ;
      return hostName ;
   }

   const CHAR* getDataNotePath ( const CHAR *my, const CHAR *other )
   {
      getDataPath ( dataPath, arrayLen, my , other ) ;
      return dataPath ;
   }

   INT32 getSourceRGName ( const CHAR *clFullName, const CHAR **tName )
   {
      INT32 rc = SDB_OK ;
      sdbCursor cursor ;
      BSONObj cond ;
      BSONObj sel ;
      BSONObj obj ;
      BSONObj subObj ;
      BSONObj ssubObj ;
      vector<BSONElement> vec ;
      string clName ;
      const char *fieldName ;

      cond = BSON ( "Name" << clFullName ) ;
      sel = BSON ( "" << "CataInfo" ) ;
      rc = db.getSnapshot ( cursor, 8, cond ) ;
      if ( rc )
         goto error ;
      rc = cursor.current ( obj ) ;
      if ( rc )
         goto error ;
      fieldName = "CataInfo" ;
      subObj = obj.getObjectField ( fieldName ) ;
      ssubObj = subObj.getObjectField ( "0" ) ;
      *tName = ssubObj.getStringField ( "GroupName" ) ;
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 buildTargetRG ( const CHAR *targetRGName, sdbShard &rg_ )
   {
      INT32 rc = SDB_OK ;
      rc = db.createShard ( targetRGName, rg_ ) ;
      if ( rc )
         goto error ;
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 buildAndStartNode ( sdbShard &rg,
                            const CHAR *host,
                            const CHAR *port,
                            const CHAR *path )
   {
      INT32 rc = SDB_OK ;
      std::map<std::string,std::string> config ;
      rc = rg.createNode ( host, port, path, config ) ;
      if ( rc )
         goto error ;
      rc = rg.start() ;
      if ( rc )
         goto error ;
   done :
      return rc ;
   error :
      goto done ;
   }

private :
   void initialize ()
   {
      INT32 rc = SDB_OK ;
      rc = db.connect ( _common.host, _common.port ) ;
      ASSERT_EQ ( rc, SDB_OK ) ;
   }

private :
   static const INT32 arrayLen = 256 ;
   CHAR hostName[256] ;
   CHAR dataPath[256] ;

   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbShard rg ;
   sdbNode node ;

public :
   common _common ;
} ;

/*
 * hash split
 */
class hashSplitTest : public testing::Test
{
protected :
   virtual void SetUp()
   {
      INT32 rc   = SDB_OK ;
      SINT64 NUM = 10000 ;
      BSONObj clInfo ;
      const CHAR *sourceRGName ;

      // connect
      rc = connectTo ( share._common.host, share._common.port,
                       share._common.user, share._common.passwd, db ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      // get cs
      rc = getCollectionSpace ( db, COLLECTION_SPACE_NAME, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      // drop cs
      rc = db.dropCollectionSpace ( COLLECTION_SPACE_NAME ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      // create cs
      rc = db.createCollectionSpace ( COLLECTION_SPACE_NAME, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      // create hash cl
      clInfo = BSON ( "ShardingKey" << BSON ( "id" << 1 ) <<
                      "ShardingType" << "hash" <<
                      "Partition" << 1024 ) ;
      rc = cs.createCollection ( COLLECTION_NAME, clInfo, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      // gen record
      rc =  genRecord ( cl, NUM ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   virtual void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = db.dropCollectionSpace ( COLLECTION_SPACE_NAME ) ;
      EXPECT_TRUE ( SDB_OK == rc ) ;
      rc = db.removeShard ( TARGETRGNAME ) ;
      EXPECT_TRUE ( SDB_OK == rc ) ;
      db.disconnect () ;
   }
protected :
   // global for all the hash split test
   split share ;
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbShard rg ;
   sdbNode node ;
} ;


/*************************************
 *           test begine             *
 *************************************/

TEST_F(hashSplitTest, split_sync_percent)
{
   INT32 rc                    = SDB_OK ;
   const CHAR *host            = NULL ;
   const CHAR *port            = NULL ;
   const CHAR *dataPath        = NULL ;
   const CHAR *sourceRGName    = NULL ;

   BSONObj cond ;
   FLOAT32 percent             = 0 ;
   SINT64 count                = 0 ;
   BSONObj empty ;
   // use to connect to data note to check
   sdb ddb ;
   sdbCollectionSpace dcs ;
   sdbCollection dcl ;

   // get host name
   host = share.getHost () ;
   ASSERT_TRUE ( NULL != host ) ;
   // get port
   port = SERVER1 ;
   // get date path
   dataPath = share.getDataNotePath ( _DATAPATH1, DATAPATH1 ) ;
   ASSERT_TRUE ( NULL != dataPath ) ;

   // create target rg
   rc = share.buildTargetRG ( TARGETRGNAME, rg  ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   // create node
   rc = share.buildAndStartNode ( rg, host, port, dataPath ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;

   // get source group name
   rc = share.getSourceRGName ( COLLECTION_FULL_NAME, &sourceRGName ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;

   // hash split by percent
   percent = 50.0 ;
   rc = cl.split( sourceRGName, TARGETRGNAME, percent ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   sleep ( 15 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // goto datanote to check
   rc = connectTo ( host, port, "", "", ddb ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( ddb, COLLECTION_SPACE_NAME, dcs ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( dcs, COLLECTION_NAME, dcl ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   // get count
   rc = dcl.getCount( count, empty ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   cout << "count is: " << count << endl ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE ( 0 != count ) ;
   ddb.disconnect () ;
}

TEST_F(hashSplitTest, split_sync_range)
{
   INT32 rc                    = SDB_OK ;
   const CHAR *host            = NULL ;
   const CHAR *port            = NULL ;
   const CHAR *dataPath        = NULL ;
   const CHAR *sourceRGName    = NULL ;

   BSONObj cond ;
   BSONObj endCond ;
   BSONObj empty ;
   SINT64 count                = 0 ;
   // use to connect to data note to check
   sdb ddb ;
   sdbCollectionSpace dcs ;
   sdbCollection dcl ;

   // get host name
   host = share.getHost () ;
   ASSERT_TRUE ( NULL != host ) ;
   // get port
   port = SERVER1 ;
   // get date path
   dataPath = share.getDataNotePath ( _DATAPATH1, DATAPATH1 ) ;
   ASSERT_TRUE ( NULL != dataPath ) ;

   // create target rg
   rc = share.buildTargetRG ( TARGETRGNAME, rg  ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   // create node
   rc = share.buildAndStartNode ( rg, host, port, dataPath ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;

   // get source group name
   rc = share.getSourceRGName ( COLLECTION_FULL_NAME, &sourceRGName ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;

   // hash split by range
   cond = BSON ( "Partition" << 0 ) ;
   endCond = BSON ( "Partition" << 512 ) ;
   rc = cl.split( sourceRGName, TARGETRGNAME, cond, endCond ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   sleep ( 15 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // goto datanote to check
   rc = connectTo ( host, port, "", "", ddb ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( ddb, COLLECTION_SPACE_NAME, dcs ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( dcs, COLLECTION_NAME, dcl ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   // get count
   rc = dcl.getCount( count, empty ) ;
   cout << "count is: " << count << endl ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE ( 0 != count ) ;
}

TEST_F(hashSplitTest, split_async_percent)
{
   INT32 rc                    = SDB_OK ;
   const CHAR *host            = NULL ;
   const CHAR *port            = NULL ;
   const CHAR *dataPath        = NULL ;
   const CHAR *sourceRGName    = NULL ;

   BSONObj cond ;
   FLOAT32 percent             = 0 ;
   SINT64 taskID               = 0 ;
   SINT64 count                = 0 ;
   BSONObj empty ;
   // use to connect to data note to check
   sdb ddb ;
   sdbCollectionSpace dcs ;
   sdbCollection dcl ;

   // get host name
   host = share.getHost () ;
   ASSERT_TRUE ( NULL != host ) ;
   // get port
   port = SERVER1 ;
   // get date path
   dataPath = share.getDataNotePath ( _DATAPATH1, DATAPATH1 ) ;
   ASSERT_TRUE ( NULL != dataPath ) ;

   // create target rg
   rc = share.buildTargetRG ( TARGETRGNAME, rg  ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   // create node
   rc = share.buildAndStartNode ( rg, host, port, dataPath ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;

   // get source group name
   rc = share.getSourceRGName ( COLLECTION_FULL_NAME, &sourceRGName ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;

   // hash split by percent
   percent = 50.0 ;
   rc = cl.splitAsync ( sourceRGName, TARGETRGNAME, percent, taskID ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   // wait task
   SINT64 taskIDs[1] = { taskID } ;
   rc = db.waitTasks ( taskIDs, 1 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;

   // goto datanote to check
   rc = connectTo ( host, port, "", "", ddb ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( ddb, COLLECTION_SPACE_NAME, dcs ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( dcs, COLLECTION_NAME, dcl ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   // get count
   rc = dcl.getCount( count, empty ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   cout << "count is: " << count << endl ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE ( 0 != count ) ;
   ddb.disconnect () ;
}

TEST_F(hashSplitTest, split_async_range)
{
   INT32 rc                    = SDB_OK ;
   const CHAR *host            = NULL ;
   const CHAR *port            = NULL ;
   const CHAR *dataPath        = NULL ;
   const CHAR *sourceRGName    = NULL ;

   SINT64 taskID               = 0 ;
   SINT64 count                = 0 ;
   SINT64 taskIDs[1] ;
   BSONObj cond ;
   BSONObj endCond ;
   BSONObj empty ;
   // use to connect to data note to check
   sdb ddb ;
   sdbCollectionSpace dcs ;
   sdbCollection dcl ;

   // get host name
   host = share.getHost () ;
   ASSERT_TRUE ( NULL != host ) ;
   // get port
   port = SERVER1 ; //58000
   // get date path
   dataPath = share.getDataNotePath ( _DATAPATH1, DATAPATH1 ) ;
   ASSERT_TRUE ( NULL != dataPath ) ;

   // create target rg
   rc = share.buildTargetRG ( TARGETRGNAME, rg  ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // create node
   rc = share.buildAndStartNode ( rg, host, port, dataPath ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;

   // get source group name
   rc = share.getSourceRGName ( COLLECTION_FULL_NAME, &sourceRGName ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;

   // hash split by range
   cond = BSON ( "Partition" << 0 ) ;
   endCond = BSON ( "Partition" << 512 ) ;
   rc = cl.splitAsync ( taskID, sourceRGName, TARGETRGNAME, cond, endCond ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;

   // wait task
   taskIDs[0] = taskID ;
   rc = db.waitTasks ( taskIDs, 1 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;

   // goto datanote to check
   rc = connectTo ( host, port, "", "", ddb ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( ddb, COLLECTION_SPACE_NAME, dcs ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection ( dcs, COLLECTION_NAME, dcl ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   ASSERT_EQ( SDB_OK, rc ) ;
   // get count
   rc = dcl.getCount( count, empty ) ;
   CHECK_MSG("%s%d\n","rc = ",rc);
   cout << "count is: " << count << endl ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE ( 0 != count ) ;
   ddb.disconnect () ;
}

INT32 _tmain ( INT32 argc, CHAR* argv[] )
{
   split_env = new splitEnvironment ;
   testing::AddGlobalTestEnvironment ( split_env ) ;
   testing::InitGoogleTest ( &argc, argv ) ;
   return RUN_ALL_TESTS () ;
}


/* I need to add 4 range split below */
