/**************************************************************
 * @Description: test case of Jira questionaire
 *					  SEQUOIADBMAINSTREAM-2593
 *       SEQUOIADBMAINSTREAM-4396
 *				     list backup and remove backup with group ID
 * @Modify     : Liang xuewang Init
 *           	  2017-08-02
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class backupTest : public testBase
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

TEST_F( backupTest, backupOfflineWithOption )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) ) 
   {
      printf( "Run mode is standalone.\n" ) ;
      return ;
   }

   // get a data group id
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   INT32 groupID ;
   bson obj ;
   bson_init( &obj ) ;
   rc = sdbListReplicaGroups( db, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to list replica groups" ;
   while( !sdbNext( cursor, &obj ) )
   {
      bson_iterator it ;
      bson_find( &it, &obj, "GroupName" ) ;
      string groupname = bson_iterator_string( &it ) ;
      if( groupname != "SYSCoord" && groupname != "SYSCatalogGroup" )
      {
         bson_find( &it, &obj, "GroupID" ) ;
         groupID = bson_iterator_int( &it ) ;
         break ;
      }
      bson_destroy( &obj ) ;
      bson_init( &obj ) ;
   }
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // backup offline with groupID
   bson option ;
   bson_init( &option ) ;
   bson_append_int( &option, "GroupID", groupID ) ;
   bson_finish( &option ) ;
   rc = sdbBackupOffline( db, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to backup offline" ;

   // list backup 
   rc = sdbListBackup( db, &option, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to list backup" ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // remove backup
   rc = sdbRemoveBackup( db, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to remove backup" ;

   sdbReleaseCursor( cursor ) ;
}

TEST_F( backupTest, backupWithOption )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) ) 
   {
      printf( "Run mode is standalone.\n" ) ;
      return ;
   }

   // get a data group id
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   string groupname ; 
   bson obj ;
   bson_init( &obj ) ;
   rc = sdbListReplicaGroups( db, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to list replica groups" ;
   
   while( !sdbNext( cursor, &obj ) )
   {
      bson_iterator it ;
      bson_find( &it, &obj, "GroupName" ) ;
      groupname = bson_iterator_string( &it ) ;
      if( groupname != "SYSCoord" && groupname != "SYSCatalogGroup" )
      {
         break ;
      }
      bson_destroy( &obj ) ;
      bson_init( &obj ) ;
   }
   bson_destroy( &obj ) ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // backup with group name
   bson option ;
   bson_init( &option ) ;
   bson_append_string( &option, "GroupName", groupname.c_str() ) ;
   bson_finish( &option ) ;
   rc = sdbBackup( db, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to backup" ;

   // list backup 
   rc = sdbListBackup( db, &option, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to list backup" ;
   rc = sdbCloseCursor( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // remove backup
   rc = sdbRemoveBackup( db, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to remove backup" ;

   sdbReleaseCursor( cursor ) ;
}
