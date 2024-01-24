/**************************************************************
 * @Description: test backup
 *               SEQUOIADBMAINSTREAM-2593
 *               seqDB-12304 : list backup and remove backup 
 *                             with group ID
 * @Modify     : Suqiang Ling
 *               2017-09-11
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include <vector>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;
using namespace sdbclient ;
using namespace std ;
using namespace bson ;

class backup12304 : public testBase
{
protected:
   INT32 getGroupID( sdb &db, INT32 &groupID )
   {
      INT32 rc = SDB_OK ;
      sdbCursor cursor ;
      BSONObj rgInfo ;

      rc = db.listReplicaGroups( cursor ) ;
      CHECK_RC( SDB_OK, rc, "fail to list replica groups" ) ;
      while( !( cursor.next( rgInfo ) ) )
      {
         string groupname = rgInfo.getField( "GroupName" ).String() ;
         if( groupname != "SYSCoord" && groupname != "SYSCatalogGroup" )
         {
            groupID = rgInfo.getField( "GroupID" ).Int() ;
            break ;
         }
      }
      rc = cursor.close() ;
      CHECK_RC( SDB_OK, rc, "fail to close cursor of list rg" ) ;
      cout << "backup group id: " << groupID << endl ;
   done:
      return rc ;
   error:
      goto done ;
   }
} ;

TEST_F( backup12304, backup )
{
   if( isStandalone( db ) )
   {
      cout << "skip this test for standalone" << endl ; 
      return ;
   }
   
   // get a data group id
   INT32 rc = SDB_OK ;
   INT32 groupID ;
   rc = getGroupID( db, groupID ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get a groupID" ;
   
   // backup offline with groupID
   string bkName = "bk12304" ;
   BSONObj option = BSON( "Name" << bkName.c_str() << "GroupID" << groupID ) ; 
   rc = db.backupOffline( option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to backup offline" ;

   // list backup to check create backup ok
   sdbCursor cursor ;
   rc = db.listBackup( cursor, option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to list backup" ;
   BSONObj bkInfo ;
   rc = cursor.next( bkInfo ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cursor of list backup" ;
   string actBkName = bkInfo.getField( "Name" ).String() ;
   ASSERT_STREQ( bkName.c_str(), actBkName.c_str() ) << "wrong backup info" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor of list backup" ;

   // remove backup
   rc = db.removeBackup( option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to remove backup" ;

   // list backup to check remove backup ok 
   rc = db.listBackup( cursor, option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to list backup" ;
   rc = cursor.next( bkInfo ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "backup info shouldn't exist" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor of list backup" ;
}
