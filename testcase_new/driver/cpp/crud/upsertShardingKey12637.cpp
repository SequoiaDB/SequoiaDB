/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12637:执行upsert更新分区键（2.9主干）
 * @Modify:        Liang xuewang Init
 *                 2017-08-29
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class upsertShardingKeyTest12637 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "upsertShardingKeyTestCs12637" ;
      clName = "upsertShardingKeyTestCl12637" ;
      rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = db.dropCollectionSpace( csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      } 
      testBase::TearDown() ;
   }
} ;

TEST_F( upsertShardingKeyTest12637, upsertMatch12637 )
{
   INT32 rc = SDB_OK ;

   // check standalone
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   // create hash split cl
   BSONObj option = BSON( "ShardingKey" << BSON( "a" << 1 ) << "ShardingType" << "hash"
                          << "ReplSize" << 0 ) ;
   rc = cs.createCollection( clName, option, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ; 

   // insert doc
   BSONObj doc = BSON( "a" << 1 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert doc" ;

   // upsert with cond match exist
   BSONObj cond = BSON( "a" << 1 ) ;
   BSONObj rule = BSON( "$set" << BSON( "a" << 10 ) ) ;
   rc = cl.upsert( rule, cond, _sdbStaticObject, _sdbStaticObject, UPDATE_KEEP_SHARDINGKEY ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to upsert" ;

   // check upsert
   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 1, count ) << "fail to check count" ;
   sdbCursor cursor ;
   rc = cl.query( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 10, obj.getField( "a" ).Int() ) << "fail to check query result" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // drop cl
   rc = cs.dropCollection( clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
}

TEST_F( upsertShardingKeyTest12637, upsertNotMatch12637 )
{
   INT32 rc = SDB_OK ;

   // check standalone
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   // create hash split cl
   BSONObj option = BSON( "ShardingKey" << BSON( "a" << 1 ) << "ShardingType" << "hash"
                          << "ReplSize" << 0 ) ;
   rc = cs.createCollection( clName, option, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ; 

   // insert doc
   BSONObj doc = BSON( "a" << 1 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert doc" ;

   // upsert with cond match not exist
   BSONObj cond = BSON( "a" << 2 ) ;
   BSONObj rule = BSON( "$set" << BSON( "a" << 10 ) ) ;
   BSONObj setOnInsert = BSON( "a" << 2 ) ;
   rc = cl.upsert( rule, cond, _sdbStaticObject, setOnInsert, UPDATE_KEEP_SHARDINGKEY ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to upsert" ;

   // check upsert
   SINT64 count ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get count" ;
   ASSERT_EQ( 2, count ) << "fail to check count" ;
   sdbCursor cursor ;
   rc = cl.query( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 2, obj.getField( "a" ).Int() ) << "fail to check query result" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // drop cl
   rc = cs.dropCollection( clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
}
