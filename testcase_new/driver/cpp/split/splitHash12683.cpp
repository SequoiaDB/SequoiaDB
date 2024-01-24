/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12683:分区表执行百分比切分（同步、异步/等待切分任务）
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

class splitHashTest12683 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   const CHAR* clFullName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "splitHashTestCs12683" ;
      clName = "splitHashTestCl12683" ;
      clFullName = "splitHashTestCs12683.splitHashTestCl12683" ;
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

TEST_F( splitHashTest12683, splitHashSync12683 )
{
   INT32 rc = SDB_OK ;
   
   // check standalone
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone." << endl ;
      return ;
   }

   // get data groups and check data group num
   vector<string> groups ;
   rc = getGroups( db, groups ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get data groups" ;
   if( groups.size() < 2 )
   {
      cout << "data group num: " << groups.size() << " too few." << endl ;
      return ;
   }

   // srcGroup dstGroup
   const char* srcGroup = groups[0].c_str() ;
   const char* dstGroup = groups[1].c_str() ;
   cout << "src group: " << srcGroup << " dst group: " << dstGroup << endl ;
   
   // create range cl in srcGroup
   BSONObj option = BSON( "ShardingKey" << BSON( "a" << 1 ) << "ShardingType" << "hash" << 
                          "Partition" << 2048 << "Group" << srcGroup << "ReplSize" << 0 ) ;
   rc = cs.createCollection( clName, option, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

   // split cl
   rc = cl.split( srcGroup, dstGroup, 50 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to split cl " << clName ;

   // check cl groups after split
   vector<string> clGroups ;
   rc = getClGroups( db, clFullName, clGroups ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl data groups" ;
   ASSERT_EQ( 2, clGroups.size() ) << "fail to check cl data groups num" ;
   ASSERT_TRUE( ( clGroups[0] == srcGroup && clGroups[1] == dstGroup ) || 
                ( clGroups[0] == dstGroup && clGroups[1] == srcGroup ) ) << "fail to check cl data groups" ;

   // check cl split percent
   sdbCursor cursor ;
   BSONObj cond = BSON( "Name" << clFullName ) ;
   rc = db.getSnapshot( cursor, SDB_SNAP_CATALOG, cond ) ;   
   ASSERT_EQ( SDB_OK, rc ) << "fail to get snapshot cata" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   vector<BSONElement> cataInfo = obj.getField( "CataInfo" ).Array() ;
   for( INT32 i = 0;i < cataInfo.size();i++ )
   {
      if( cataInfo[i].Obj().getField( "GroupName" ).String() == dstGroup )
      {
         INT32 lowBound = cataInfo[i].Obj().getField( "LowBound" ).Obj().getField( "" ).Int() ;
         INT32 upBound = cataInfo[i].Obj().getField( "UpBound" ).Obj().getField( "" ).Int() ;
         ASSERT_EQ( 1024, lowBound ) << "fail to check lowBound" ;
         ASSERT_EQ( 2048, upBound ) << "fail to check upBound " ;
         break ;
      }
   }
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}

TEST_F( splitHashTest12683, splitHashAsync12683 )
{
   INT32 rc = SDB_OK ;
   
   // check standalone
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone." << endl ;
      return ;
   }

   // get data groups and check data group num
   vector<string> groups ;
   rc = getGroups( db, groups ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get data groups" ;
   if( groups.size() < 2 )
   {
      cout << "data group num: " << groups.size() << " too few." << endl ;
      return ;
   }

   // srcGroup dstGroup
   const char* srcGroup = groups[0].c_str() ;
   const char* dstGroup = groups[1].c_str() ;
   cout << "src group: " << srcGroup << " dst group: " << dstGroup << endl ;
   
   // create hash cl in srcGroup
   BSONObj option = BSON( "ShardingKey" << BSON( "a" << 1 ) << "ShardingType" << "hash" << 
                          "Partition" << 2048 << "Group" << srcGroup << "ReplSize" << 0 ) ;
   rc = cs.createCollection( clName, option, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

   // split cl async
   SINT64 taskID ;
   rc = cl.splitAsync( srcGroup, dstGroup, 50, taskID ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to async split cl " << clName ;

   // wait task
   SINT64 taskIDs[1] ;
   taskIDs[0] = taskID ;
   rc = db.waitTasks( taskIDs, 1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to wait task " << taskID ;

   // check cl groups after split
   vector<string> clGroups ;
   rc = getClGroups( db, clFullName, clGroups ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl data groups" ;
   ASSERT_EQ( 2, clGroups.size() ) << "fail to check cl data groups num" ;
   ASSERT_TRUE( ( clGroups[0] == srcGroup && clGroups[1] == dstGroup ) || 
                ( clGroups[0] == dstGroup && clGroups[1] == srcGroup ) ) << "fail to check cl data groups" ;

   // check cl split condition
   sdbCursor cursor ;
   BSONObj cond = BSON( "Name" << clFullName ) ;
   rc = db.getSnapshot( cursor, SDB_SNAP_CATALOG, cond ) ;   
   ASSERT_EQ( SDB_OK, rc ) << "fail to get snapshot cata" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   vector<BSONElement> cataInfo = obj.getField( "CataInfo" ).Array() ;
   for( INT32 i = 0;i < cataInfo.size();i++ )
   {
      if( cataInfo[i].Obj().getField( "GroupName" ).String() == dstGroup )
      {
         INT32 lowBound = cataInfo[i].Obj().getField( "LowBound" ).Obj().getField( "" ).Int() ;
         INT32 upBound = cataInfo[i].Obj().getField( "UpBound" ).Obj().getField( "" ).Int() ;
         ASSERT_EQ( 1024, lowBound ) << "fail to check lowBound" ;
         ASSERT_EQ( 2048, upBound ) << "fail to check upBound " ;
         break ;
      }
   }
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}
