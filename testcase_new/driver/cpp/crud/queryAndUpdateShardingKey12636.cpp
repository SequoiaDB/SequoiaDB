/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12636:执行QueryAndUpdate更新分区键
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

class queryAndUpdateShardingKeyTest12636 : public testBase
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
      csName = "queryAndUpdateShardingKeyTestCs12636" ;
      clName = "queryAndUpdateShardingKeyTestCl12636" ;
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

TEST_F( queryAndUpdateShardingKeyTest12636, queryAndUpdate12636 )
{
   INT32 rc = SDB_OK ;

   // check standalone
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   // create split cl
   BSONObj option = BSON( "ShardingKey" << BSON( "a" << 1 ) << "ShardingType" << "hash" <<
                          "ReplSize" << 0 ) ;
   rc = cs.createCollection( clName, option, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

   // insert doc
   BSONObj doc = BSON( "a" << 1 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   
   // queryAndUpdate ShardingKey
   BSONObj cond = BSON( "a" << 1 ) ;
   BSONObj update = BSON( "$set" << BSON( "a" << 10 ) ) ;
   sdbCursor cursor ;
   rc = cl.queryAndUpdate( cursor, update, cond, _sdbStaticObject, _sdbStaticObject, _sdbStaticObject,
                           0, -1, QUERY_KEEP_SHARDINGKEY_IN_UPDATE ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to queryAndUpdate with flag " << QUERY_KEEP_SHARDINGKEY_IN_UPDATE ;

   // query before cursor next
   sdbCursor cursor1 ;
   BSONObj obj ; 
   rc = cl.query( cursor1, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = cursor1.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 1, obj.getField( "a" ).Int() ) << "fail to check a value" ;
   rc = cursor1.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor1" ;

   // cursor next
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 1, obj.getField( "a" ).Int() ) << "fail to check a value" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;  

   // query after cursor next
   rc = cl.query( cursor1, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = cursor1.next( obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to test query after queryAndRemove" ;
   rc = cursor1.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor1" ;

   // query new doc
   cond = BSON( "a" << 10 ) ;
   rc = cl.query( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 10, obj.getField( "a" ).Int() ) << "fail to check a value" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
} 
