/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12538:setSessionAttr设置会话属性
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

class setSessionAttrTest12538 : public testBase
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
      csName = "setSessionAttrTestCs12538" ;
      clName = "setSessionAttrTestCl12538" ;
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

// 设置会话从主节点读取，检查协调节点日志验证查询走主节点
TEST_F( setSessionAttrTest12538, master12538 )
{
   INT32 rc = SDB_OK ;

   // check standalone
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   // get data groups and create cl in a data group
   vector<string> groups ;
   rc = getGroups( db, groups ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get data groups" ;
   const char* groupName = groups[0].c_str() ;
   cout << "group: " << groupName << endl ;
   BSONObj option = BSON( "Group" << groupName ) ;
   rc = cs.createCollection( clName, option, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

   // get data group master node
   sdbReplicaGroup rg ;
   sdbNode master ;
   rc = db.getReplicaGroup( groupName, rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get group " << groupName ;
   rc = rg.getMaster( master ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get master node" ;
   cout << "master node: " << master.getNodeName() << endl ;

   // setSessionAttr PreferInstance m
   option = BSON( "PreferedInstance" << "m" ) ;
   rc = db.setSessionAttr( option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to setSessionAttr" ;

   // query, check coord diaglog
   sdbCursor cursor ;
   rc = cl.query( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   
   // drop cl
   rc = cs.dropCollection( clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
}
