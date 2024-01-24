/**************************************************************************
 * @Description:   test case for C++ driver
 *                 :集合版本号校验
 * @Modify:        Xia qianyong Init
 *                 2020-0-11
 **************************************************************************/
#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

#define FIELD_NAME_ALTER_TYPE                "AlterType"
#define SDB_CATALOG_CS                       "collection space"
#define SDB_CATALOG_CL                       "collection"
#define FIELD_NAME_VERSION                   "Version"
#define FIELD_NAME_NAME                      "Name"
#define FIELD_NAME_ALTER                     "Alter"
#define SDB_ALTER_ACTION_INC_VER             "increase version"
#define SDB_ALTER_CL_INC_VER                 SDB_ALTER_ACTION_INC_VER
#define FIELD_NAME_CHECK_CLIENT_CATA_VERSION "CheckClientCataVersion"
#define SDB_ALTER_VERSION                     1

#define CATALOG_INVALID_CHECK_VERSION        -2


class clVersionCheckTest : public testing::Test
{
protected:
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   const CHAR *csName ;
   const CHAR *clName ;
   const CHAR *clNewName ;
   const CHAR *clFullName ;

   void SetUp()
   {
      INT32 rc = SDB_OK ;
      const CHAR *hostName = "localhost"; //HOST ;
      const CHAR *svcPort = SERVER ;
      const CHAR *usr = USER ;
      const CHAR *passwd = PASSWD ;
      rc = db.connect( hostName, svcPort, usr, passwd ) ;

      csName = "clVerCheckTestCs" ;
      clName = "clVerCheckTestCl" ;
      clNewName = "clVerCheckTestNewCl" ;
      clFullName = "clVerCheckTestCs.clVerCheckTestCl";

      rc = clVerCheckCreateCsCl( db, cs, cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;
   }

   void TearDown()
   {
      INT32 rc = db.dropCollectionSpace( csName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      db.disconnect();
   }

   INT32 clVerCheckCreateCsCl( sdb& db,
                           sdbCollectionSpace& cs,
                           sdbCollection& cl,
                           const CHAR* csName,
                           const CHAR* clName )
   {
      INT32 rc = SDB_OK ;

      rc = db.createCollectionSpace( csName, 4096, cs ) ;
      if( SDB_DMS_CS_EXIST == rc )
      {
         rc = db.getCollectionSpace( csName, cs ) ;
      }
      rc = cs.createCollection( clName, cl ) ;
      if( SDB_DMS_EXIST == rc )
      {
         rc = cs.dropCollection( clName ) ;
         rc = cs.createCollection( clName, cl ) ;
      }
   }

} ;

TEST_F( clVersionCheckTest, getCollection_then_checkVersion )
{
   INT32 rc = SDB_OK ;
   sdbCollection getCl ;

   rc = cs.getCollection( clName, getCl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get collection version" ;
   ASSERT_GT(getCl.getVersion(), CATALOG_INVALID_CHECK_VERSION);
}

TEST_F( clVersionCheckTest, rename_then_getCollection )
{
   INT32 rc = SDB_OK ;
   sdbCollection getCl ;

   rc = cs.getCollection( clName, getCl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get collection version" ;
   INT32 oldVersion = getCl.getVersion( );
   ASSERT_GT(oldVersion, CATALOG_INVALID_CHECK_VERSION);

   rc = cs.renameCollection( clName, clNewName );
   ASSERT_EQ( SDB_OK, rc ) << "fail to rename collection" ;
   rc = cs.getCollection( clNewName, getCl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get collection version" ;
   INT32 newVersion = getCl.getVersion( );
   ASSERT_GT(newVersion, oldVersion);
}

TEST_F( clVersionCheckTest, insert_default )
{
   INT32 rc = SDB_OK ;
   BSONObj insertObj ;

   cl.setVersion( CATALOG_INVALID_CHECK_VERSION );
   ASSERT_EQ( SDB_OK, rc ) << "fail to set collection version -1" ;
   ASSERT_EQ(CATALOG_INVALID_CHECK_VERSION,cl.getVersion());

   // insert no version check
   insertObj = BSON( "_id" << 1 << "a" << 1 ) ;
   rc = cl.insert( insertObj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
}

TEST_F( clVersionCheckTest, insert_check_true_invalid_version )
{
   INT32 rc = SDB_OK ;
   BSONObj checkObj ;
   BSONObj insertObj ;

   // insert version check with version:-1 to sdb
   cl.setVersion( CATALOG_INVALID_CHECK_VERSION );
   ASSERT_EQ( SDB_OK, rc ) << "fail to set collection version -1" ;
   ASSERT_EQ(CATALOG_INVALID_CHECK_VERSION,cl.getVersion());

   checkObj = BSON(FIELD_NAME_CHECK_CLIENT_CATA_VERSION << true);
   rc = db.setSessionAttr(checkObj);
   ASSERT_EQ( SDB_OK, rc ) << "fail to set check cat version true" ;

   insertObj = BSON( "_id" << 2 << "a" << 2 ) ;
   rc = cl.insert( insertObj ) ;
   ASSERT_EQ( SDB_CLIENT_CATA_VER_OLD, rc ) << "fail to insert"
              "with version check expect rc SDB_CLIENT_CATA_VER_OLD" ;
   ASSERT_NE(CATALOG_INVALID_CHECK_VERSION,cl.getVersion());
}

TEST_F( clVersionCheckTest, insert_check_repeat )
{
   INT32 rc = SDB_OK ;
   BSONObj checkObj ;
   BSONObj insertObj ;
   BSONObj errObj ;
   // insert version check with version:-1 to sdb
   cl.setVersion( CATALOG_INVALID_CHECK_VERSION );
   ASSERT_EQ(CATALOG_INVALID_CHECK_VERSION,cl.getVersion());

   checkObj = BSON(FIELD_NAME_CHECK_CLIENT_CATA_VERSION << true);
   rc = db.setSessionAttr(checkObj);
   ASSERT_EQ( SDB_OK, rc ) << "fail to set check cat version true" ;

   insertObj = BSON( "_id" << 3 << "a" << 3 ) ;
   rc = cl.insert( insertObj ) ;

   ASSERT_EQ( SDB_CLIENT_CATA_VER_OLD, rc ) << "fail to insert"
              "with version check expect rc SDB_CLIENT_CATA_VER_OLD" ;

   ASSERT_NE(CATALOG_INVALID_CHECK_VERSION,cl.getVersion());

   insertObj = BSON( "_id" << 3 << "a" << 3 ) ;
   rc = cl.insert( insertObj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert check repeat" ;
}

TEST_F( clVersionCheckTest, update_default )
{
   INT32 rc = SDB_OK ;
   BSONObj updateObj ;
   BSONObj cond ;

   cl.setVersion( CATALOG_INVALID_CHECK_VERSION );
   ASSERT_EQ(CATALOG_INVALID_CHECK_VERSION,cl.getVersion());

   // update no version check
   cond = BSON( "_id" << 1 ) ;
   updateObj = BSON( "$inc" << BSON( "a" << 1 ) ) ;
   rc = cl.update( updateObj, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to update" ;
}

TEST_F( clVersionCheckTest, update_check_repeat)
{
   INT32 rc = SDB_OK ;
   BSONObj checkObj ;
   BSONObj updateObj ;
   BSONObj cond ;
   BSONObj errObj ;

   cl.setVersion( CATALOG_INVALID_CHECK_VERSION );
   ASSERT_EQ(CATALOG_INVALID_CHECK_VERSION,cl.getVersion());

   // update version check with version:-1 to sdb
   checkObj = BSON(FIELD_NAME_CHECK_CLIENT_CATA_VERSION << true);
   rc = db.setSessionAttr(checkObj);
   ASSERT_EQ( SDB_OK, rc ) << "fail to set check cat version true" ;

   cond = BSON( "_id" << 1 ) ;
   updateObj = BSON( "$inc" << BSON( "a" << 5 ) ) ;
   rc = cl.update( updateObj, cond ) ;
   ASSERT_EQ( SDB_CLIENT_CATA_VER_OLD, rc ) << "fail to update"
              "with version check expect rc SDB_CLIENT_CATA_VER_OLD" ;

   ASSERT_NE(CATALOG_INVALID_CHECK_VERSION,cl.getVersion());
   // update version check with version:last sdb send back version
   cond = BSON( "_id" << 1 ) ;
   updateObj = BSON( "$inc" << BSON( "a" << 5 ) ) ;
   rc = cl.update( updateObj, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to update repeat" ;
}

TEST_F( clVersionCheckTest, upsert_check)
{
   INT32 rc = SDB_OK ;
   BSONObj checkObj ;
   BSONObj updateObj ;
   BSONObj cond ;
   BSONObj errObj ;

   cl.setVersion( CATALOG_INVALID_CHECK_VERSION );
   ASSERT_EQ(CATALOG_INVALID_CHECK_VERSION,cl.getVersion());

   // upsert version check with version:-1 to sdb
   checkObj = BSON(FIELD_NAME_CHECK_CLIENT_CATA_VERSION << true);
   rc = db.setSessionAttr(checkObj);
   ASSERT_EQ( SDB_OK, rc ) << "fail to set check cat version true" ;

   cond = BSON( "_id" << 1 ) ;
   updateObj = BSON( "$inc" << BSON( "a" << 5 ) ) ;
   rc = cl.upsert( updateObj, cond ) ;
   ASSERT_EQ( SDB_CLIENT_CATA_VER_OLD, rc ) << "fail to update"
              "with version check expect rc SDB_CLIENT_CATA_VER_OLD" ;

   ASSERT_NE(CATALOG_INVALID_CHECK_VERSION,cl.getVersion());
   // upsert version check with version:last sdb send back version
   cond = BSON( "_id" << 1 ) ;
   updateObj = BSON( "$inc" << BSON( "a" << 5 ) ) ;
   rc = cl.upsert( updateObj, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to upsert repeat" ;

   // upsert version check with version:last sdb send back version
   cond = BSON( "_id" << 1 ) ;
   updateObj = BSON( "$inc" << BSON( "a" << 5 ) ) ;
   rc = cl.upsert( updateObj, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to upsert repeat" ;
}

TEST_F( clVersionCheckTest, delete_default )
{
   INT32 rc = SDB_OK ;
   BSONObj checkObj ;
   BSONObj cond ;

   cl.setVersion( CATALOG_INVALID_CHECK_VERSION );
   ASSERT_EQ(CATALOG_INVALID_CHECK_VERSION,cl.getVersion());

   // delete no version check
   cond = BSON( "_id" << 1 ) ;
   rc = cl.del( cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to del" ;
}

TEST_F( clVersionCheckTest, delete_check_true )
{
   INT32 rc = SDB_OK ;
   BSONObj checkObj ;
   BSONObj cond ;

   cl.setVersion( CATALOG_INVALID_CHECK_VERSION );
   ASSERT_EQ(CATALOG_INVALID_CHECK_VERSION,cl.getVersion());

   // query with version check
   checkObj = BSON(FIELD_NAME_CHECK_CLIENT_CATA_VERSION << true);
   rc = db.setSessionAttr(checkObj);
   ASSERT_EQ( SDB_OK, rc ) << "fail to set check cat version true" ;

   sdbCursor cursor ;
   cond = BSON( "_id" << 1 ) ;
   rc = cl.query( cursor, cond ) ;
   ASSERT_EQ( SDB_CLIENT_CATA_VER_OLD, rc ) << "fail to query"
              "with version check expect rc SDB_CLIENT_CATA_VER_OLD" ;
}
TEST_F( clVersionCheckTest, delete_check_false )
{
   INT32 rc = SDB_OK ;
   BSONObj checkObj ;
   BSONObj cond ;

   // no version check
	checkObj = BSON(FIELD_NAME_CHECK_CLIENT_CATA_VERSION << false);
	rc = db.setSessionAttr(checkObj);
	ASSERT_EQ( SDB_OK, rc ) << "fail to set check cat version true" ;

   sdbCursor cursor ;
   cond = BSON( "_id" << 1 ) ;
	rc = cl.query( cursor, cond ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;

   BSONObj cursorObj ;
   rc = cursor.next( cursorObj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to check del" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}



TEST_F( clVersionCheckTest, increase_version)
{
   INT32 rc = SDB_OK ;
   BSONObj increaseVerObj ;

   increaseVerObj = BSON( FIELD_NAME_ALTER_TYPE << SDB_CATALOG_CL
                       << FIELD_NAME_VERSION << SDB_ALTER_VERSION
                       << FIELD_NAME_NAME << clFullName
                       << FIELD_NAME_ALTER << BSON( FIELD_NAME_NAME<<SDB_ALTER_CL_INC_VER));

   rc = cl.alterCollection(increaseVerObj);
   ASSERT_EQ( SDB_OK, rc ) << "fail to increase version" ;
   INT32 newVersion1 = cl.getVersion( );

   rc = cl.alterCollection(increaseVerObj);
   ASSERT_EQ( SDB_OK, rc ) << "fail to increase version" ;
   INT32 newVersion2 = cl.getVersion( );

   ASSERT_GT(newVersion2, newVersion1);
   ASSERT_GT(newVersion1,0);
}

TEST_F( clVersionCheckTest, increase_dml_return_currentVersion )
{
   INT32 rc = SDB_OK ;
   BSONObj increaseVerObj ;
   BSONObj checkObj;
   BSONObj cond ;
   BSONObj errObj ;

   //increase version
   increaseVerObj = BSON( FIELD_NAME_ALTER_TYPE << SDB_CATALOG_CL 
                       << FIELD_NAME_VERSION << SDB_ALTER_VERSION 
                       << FIELD_NAME_NAME << clFullName
                       << FIELD_NAME_ALTER << BSON( FIELD_NAME_NAME<<SDB_ALTER_CL_INC_VER));

   rc = cl.alterCollection(increaseVerObj);
   ASSERT_EQ( SDB_OK, rc ) << "fail to increase version" ;
   INT32 preVersion = cl.getVersion( );
   ASSERT_GT(preVersion,0);


   // query version check failed errObj contain new version
   cl.setVersion( CATALOG_INVALID_CHECK_VERSION );
   ASSERT_EQ(CATALOG_INVALID_CHECK_VERSION,cl.getVersion());

   checkObj = BSON(FIELD_NAME_CHECK_CLIENT_CATA_VERSION << true);
   rc = db.setSessionAttr(checkObj);
   ASSERT_EQ( SDB_OK, rc ) << "fail to set check cat version true" ;

   sdbCursor cursor ;
   cond = BSON( "_id" << 1 ) ;
   rc = cl.query( cursor, cond ) ;
   ASSERT_EQ( SDB_CLIENT_CATA_VER_OLD, rc ) << "fail to query"
              "with version check expect rc SDB_CLIENT_CATA_VER_OLD" ;

   ASSERT_EQ(cl.getVersion(),preVersion);

}

TEST_F( clVersionCheckTest, increase_truncate )
{
   INT32 rc = SDB_OK ;
   BSONObj increaseVerObj ;

   //increase version
   increaseVerObj = BSON( FIELD_NAME_ALTER_TYPE << SDB_CATALOG_CL
                       << FIELD_NAME_VERSION << SDB_ALTER_VERSION
                       << FIELD_NAME_NAME << clFullName
                       << FIELD_NAME_ALTER << BSON( FIELD_NAME_NAME<<SDB_ALTER_CL_INC_VER));

   rc = cl.alterCollection(increaseVerObj);
   ASSERT_EQ( SDB_OK, rc ) << "fail to increase version" ;
   INT32 newVersion = cl.getVersion( );
   ASSERT_GT(newVersion,0);

   rc = cl.truncate() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_GE( cl.getVersion(), newVersion );

}

TEST_F(clVersionCheckTest,aggregate_dml_return_currentVersion)
{
   INT32 rc = SDB_OK;
   sdbCursor cursor ;
   BSONObj obj ;
   BSONObj checkObj ;
   vector<BSONObj> ob ;

   int iNUM = 2 ;
   int rNUM = 4 ;
   int i = 0 ;
   const char* command[iNUM] ;
   const char* record[rNUM] ;
   command[0] = "{$match:{status:\"A\"}}" ;
   command[1] = "{$group:{_id:\"$cust_id\",total:{$sum:\"$amount\"}}}" ;
   record[0] = "{cust_id:\"A123\",amount:500,status:\"A\"}" ;
   record[1] = "{cust_id:\"A123\",amount:250,status:\"A\"}" ;
   record[2] = "{cust_id:\"B212\",amount:200,status:\"A\"}" ;
   record[3] = "{cust_id:\"A123\",amount:300,status:\"D\"}" ;
   const char* m = "{$match:{status:\"A\"}}" ;
   const char* g = "{$group:{_id:\"$cust_id\",total:{$sum:\"$amount\"}}}" ;

   // insert record
   for( i=0; i<rNUM; i++ )
   {
      rc = fromjson( record[i], obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = cl.insert( obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   // build bson vector
   for ( i=0; i<iNUM; i++ )
   {
      rc = fromjson( command[i], obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      ob.push_back( obj ) ;
   }

   // 
   INT32 preVersion = cl.getVersion();
   cl.setVersion( CATALOG_INVALID_CHECK_VERSION );
   ASSERT_EQ(CATALOG_INVALID_CHECK_VERSION,cl.getVersion());

   checkObj = BSON(FIELD_NAME_CHECK_CLIENT_CATA_VERSION << true);
   rc = db.setSessionAttr(checkObj);
   ASSERT_EQ( SDB_OK, rc ) << "fail to set check cat version true" ;

   // aggregate
   rc = cl.aggregate( cursor, ob ) ;
   ASSERT_EQ( SDB_CLIENT_CATA_VER_OLD, rc ) << "fail to aggregate"
              "with version check expect rc SDB_CLIENT_CATA_VER_OLD" ;
   ASSERT_EQ(cl.getVersion(),preVersion);

   // aggregate again
   rc = cl.aggregate( cursor, ob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to aggregate repeat"
              "with version check expect rc SDB_CLIENT_CATA_VER_OLD" ;
}


INT32 _tmain ( INT32 argc, CHAR* argv[] )
{
   testing::InitGoogleTest ( &argc, argv ) ;
   return RUN_ALL_TESTS () ;
}



