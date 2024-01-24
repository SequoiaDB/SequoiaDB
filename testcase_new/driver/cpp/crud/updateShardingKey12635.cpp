/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12635:执行update更新分区键
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

class updateShardingKeyTest12635 : public testBase
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
      csName = "updateShardingKeyTestCs12635" ;
      clName = "updateShardingKeyTestCl12635" ;
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

TEST_F( updateShardingKeyTest12635, update12635 )
{
   INT32 rc = SDB_OK ;

   // check standalone
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }
   
   // get data groups
   vector<string> groups ;
   rc = getGroups( db, groups ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   if( groups.size() < 2 )
   {
      cout << "data group num: " << groups.size() << " too few" << endl ;
      return ; 
   }
   const CHAR* srcGroup = groups[0].c_str() ;
   const CHAR* dstGroup = groups[1].c_str() ;

   // create split cl
   BSONObj option = BSON( "ShardingKey" << BSON( "a" << 1 ) << "ShardingType" << "range" <<
                          "Group" << srcGroup << "ReplSize" << 0 ) ;
   rc = cs.createCollection( clName, option, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

   // insert doc
   BSONObj doc = BSON( "a" << 1 << "b" << 1 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   
   // update ShardingKey before split
   BSONObj cond = BSON( "a" << 1 ) ;
   BSONObj rule = BSON( "$set" << BSON( "a" << 10 << "b" << 10 ) ) ;
   rc = cl.update( rule, cond, _sdbStaticObject, UPDATE_KEEP_SHARDINGKEY ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to update" ;

   // check update
   sdbCursor cursor ;
   cond = BSON( "a" << 10 ) ;
   rc = cl.query( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 10, obj.getField( "a" ).Int() ) << "fail to check a" ;
   ASSERT_EQ( 10, obj.getField( "b" ).Int() ) << "fail to check b" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor1" ;

   // split cl
   BSONObj begin = BSON( "a" << 10 ) ;
   BSONObj end = BSON( "a" << 30 ) ;
   rc = cl.split( srcGroup, dstGroup, begin, end ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to split" ;

   // update ShardingKey after split
   cond = BSON( "a" << 10 ) ;
   rule = BSON( "$set" << BSON( "a" << 15 ) ) ;
   rc = cl.update( rule, cond, _sdbStaticObject, UPDATE_KEEP_SHARDINGKEY ) ;
   ASSERT_EQ( SDB_UPDATE_SHARD_KEY, rc ) << "fail to test update after split" ;
}
