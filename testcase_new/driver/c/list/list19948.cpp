/**************************************************************************
 * @Description :   test sdbGetList
 * @Modify      :   liuxiaoxuan
 * @testlink    :   seqDB-19948
 *                  2019-10-10
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class list19948 : public testBase
{
protected:
   const CHAR* tmpUsr1 ;
   const CHAR* tmpPasswd1 ;
   const CHAR* tmpUsr2 ;
   const CHAR* tmpPasswd2 ;
   const CHAR* tmpUsr3 ;
   const CHAR* tmpPasswd3 ; 
   const CHAR* backupName ; 

   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      ASSERT_EQ( SDB_OK, rc ) ;

      if( isStandalone( db ) )
      {
         printf( "Run mode is standalone\n" ) ;
         return ;
      }

      // create user
      tmpUsr1 = "test1" ;
      tmpPasswd1 = "test1" ;
      tmpUsr2 = "test2" ;
      tmpPasswd2 = "test2" ;
      tmpUsr3 = "test3" ;
      tmpPasswd3 = "test3" ;
      rc = sdbCreateUsr( db, tmpUsr1, tmpPasswd1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbCreateUsr( db, tmpUsr2, tmpPasswd2 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = sdbCreateUsr( db, tmpUsr3, tmpPasswd3 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;

      // backup
      backupName = "backup19948" ;
      sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
      bson ret ;
      bson_init( &ret ) ; 
      rc = sdbListReplicaGroups( db, &cursor ) ; 
      ASSERT_EQ( SDB_OK, rc ) ; 
      string groupName ;
      while( !sdbNext( cursor, &ret ) ) 
      {   
         bson_iterator it ;
         bson_find( &it, &ret, "GroupName" ) ; 
         groupName = bson_iterator_string( &it ) ; 
         if( groupName != "SYSCoord" && groupName != "SYSCatalogGroup" )
         {   
             break ;
         }   
         bson_destroy( &ret ) ; 
         bson_init( &ret ) ; 
      }   
      bson_destroy( &ret ) ; 
      sdbCloseCursor( cursor ) ;
      sdbReleaseCursor( cursor ) ; 
      bson option ;
      bson_init( &option ) ; 
      bson_append_string( &option, "Name", backupName ) ; 
      bson_append_string( &option, "GroupName", groupName.c_str() ) ; 
      bson_finish( &option ) ; 
      rc = sdbBackup( db, &option ) ; 
      ASSERT_EQ( SDB_OK, rc ) ;
      bson_destroy( &option ) ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;

      if( !isStandalone( db ) )
      {
          // remove user
          rc = sdbRemoveUsr( db, tmpUsr1, tmpPasswd1 ) ;
          ASSERT_EQ( SDB_OK, rc ) ;
          rc = sdbRemoveUsr( db, tmpUsr2, tmpPasswd2 ) ;
          ASSERT_EQ( SDB_OK, rc ) ;
          rc = sdbRemoveUsr( db, tmpUsr3, tmpPasswd3 ) ;
          ASSERT_EQ( SDB_OK, rc ) ;
          // remove backup
          bson option ;
          bson_init( &option ) ; 
          bson_append_string( &option, "Name", backupName ) ; 
          bson_finish( &option ) ; 
          rc = sdbRemoveBackup( db, &option ) ;
          ASSERT_EQ( SDB_OK, rc ) ;
          bson_destroy( &option ) ;
      }
      testBase::TearDown() ;
   }
} ;

TEST_F( list19948, list_user )
{
   INT32 rc = SDB_OK ;

   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   // get list user
   INT32 listType = SDB_LIST_USERS ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson cond ;
   bson selector ;
   bson orderby ;
   bson hint ;
   bson_init( &cond ) ;
   bson_init( &selector ) ;
   bson_init( &orderby ) ;
   bson_init( &hint ) ;
   bson_append_start_object( &cond, "User" ) ;
   bson_append_int( &cond, "$isnull", 0 ) ;
   bson_append_finish_object( &cond ) ;
   bson_append_string( &selector, "User", "" ) ;
   bson_append_int( &orderby, "User", 1 ) ;
   bson_finish( &cond );
   bson_finish( &selector );
   bson_finish( &orderby );
   bson_finish( &hint );
   rc = sdbGetList1( db, listType, &cond, &selector, &orderby, &hint, 1, 2, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &cond );
   bson_destroy( &selector );
   bson_destroy( &orderby );
   bson_destroy( &hint );
 
   // check result
   bson ret ;
   bson_init( &ret ) ;
   rc = sdbNext( cursor, &ret ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   bson_iterator it ;
   bson_find( &it, &ret, "User" ) ;
   ASSERT_STREQ( "test2", bson_iterator_string( &it ) ) ;
   rc = sdbNext( cursor, &ret ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   bson_find( &it, &ret, "User" ) ;
   ASSERT_STREQ( "test3", bson_iterator_string( &it ) ) ;
   rc = sdbNext( cursor, &ret ) ;
   ASSERT_EQ( rc, SDB_DMS_EOC ) ;
   bson_destroy( &ret ) ;
   sdbCloseCursor( cursor ) ;
   sdbReleaseCursor( cursor ) ;
}

TEST_F( list19948, list_backups )
{
   INT32 rc = SDB_OK ;

   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   // get list backup
   bson cond ;
   bson_init( &cond ) ;
   bson_append_string( &cond, "Name", backupName ) ;
   bson_finish( &cond ) ;
   INT32 listType = SDB_LIST_BACKUPS ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   rc = sdbGetList1( db, listType, &cond, NULL, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &cond ) ;

   INT32 count = 0 ;
   bson ret ;
   bson_init( &ret ) ; 
   while( !sdbNext( cursor, &ret ) ) 
   {   
      count++ ;
      bson_iterator it ;
      bson_find( &it, &ret, "Name" ) ; 
      string val = bson_iterator_string( &it ) ; 
      ASSERT_EQ( backupName, val ) ; 
      bson_destroy( &ret ) ; 
      bson_init( &ret ) ; 
   }    
   ASSERT_EQ( 1, count ) ;
   bson_destroy( &ret ) ; 
   sdbCloseCursor( cursor ) ; 
   sdbReleaseCursor( cursor ) ;    
}
