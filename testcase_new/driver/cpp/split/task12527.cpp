/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12527:枚举/取消后台任务
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

class taskTest12527 : public testBase
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
      csName = "taskTestCs12527" ;
      clName = "taskTestCl12527" ;
      clFullName = "taskTestCs12527.taskTestCl12527" ;
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

TEST_F( taskTest12527, listCancelTask12527 )
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

   // list tasks
   BSONObj cond = BSON( "TaskID" << taskID ) ;
   sdbCursor cursor ;
   rc = db.listTasks( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to list tasks" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( taskID, obj.getField( "TaskID" ).Long() ) << "fail to check TaskID" ;
   ASSERT_EQ( srcGroup, obj.getField( "Source" ).String() ) << "fail to check srcGroup" ;
   ASSERT_EQ( dstGroup, obj.getField( "Target" ).String() ) << "fail to check dstGroup" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // cancel task
   rc = db.cancelTask( taskID, false ) ;   // error
   if ( rc == SDB_OK )
   {
      ASSERT_EQ( SDB_OK, rc ) << "fail to cancel task " << taskID ;
   }
   else
   {
      ASSERT_EQ( SDB_TASK_ALREADY_FINISHED, rc ) << "fail to cancel task " << taskID ;
   }

   // check cancel task
   INT32 rc1 = db.listTasks( cursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc1 ) << "fail to list tasks" ;
   rc1 = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc1 ) << "fail to list tasks" ;
   
   INT32 resCode = obj.getField("ResultCode").Int() ;
   if ( rc == SDB_OK )
   {
      ASSERT_EQ( SDB_TASK_HAS_CANCELED , resCode ) << "fail to list tasks" << obj.toString() ;
   }
   else
   {
      ASSERT_EQ( SDB_OK , resCode ) << "fail to list tasks" << obj.toString() ;
   }
  

   // check cl split 
   vector<string> clGroups ;
   rc = getClGroups( db, clFullName, clGroups ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl data groups" ;
   ASSERT_EQ( 1, clGroups.size() ) << "fail to check cl data groups num" ;
   ASSERT_EQ( srcGroup, clGroups[0] ) << "fail to check cl data groups" ;
}
