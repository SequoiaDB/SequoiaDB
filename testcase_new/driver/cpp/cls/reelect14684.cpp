/**************************************************************
 * @Description: test case for C++ driver
 *				     seqDB-14684:reelect切换数据组的主节点
 * @Modify     : Liang xuewang Init
 *			 	     2018-03-13
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace bson ;
using namespace sdbclient ;
using namespace std ;

class reelectTest : public testBase
{
protected:
   void SetUp()
   {
      testBase::SetUp() ;
   }
   void TearDown()
   {
      testBase::TearDown() ;
   }
} ;

TEST_F( reelectTest, reelect )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }
   INT32 rc = SDB_OK ;
   vector<string> groups ;
   rc = getGroups( db, groups ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const CHAR* rgName = NULL ;
   sdbReplicaGroup rg ;
   sdbNode master ;
   for( INT32 i = 0;i < groups.size();i++ )
   {
      vector<string> nodes ;
      const CHAR* tmpName = groups[i].c_str() ;
      rc = getGroupNodes( db, tmpName, nodes ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      if( nodes.size() >= 2 )
      {
         rgName = tmpName ;
         break ;
      }
   }
   if( rgName == NULL )
   {
      cout << "No group has more than 1 nodes" << endl ;
      return ;
   }
   rc = db.getReplicaGroup( rgName, rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get rg" ;

   rc = rg.getMaster( master ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get master" ;
   cout << "Before reelect, master: " << master.getNodeName() << endl ;

   rc = waitSync( db, rgName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObj option = BSON( "Seconds" << 90 ) ;
   rc = rg.reelect( option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to reelect" ;

   rc = rg.getMaster( master ) ;      
   ASSERT_EQ( SDB_OK, rc ) << "fail to get master after reelect" ;
   cout << "After reelect, master: " << master.getNodeName() << endl ;
}
