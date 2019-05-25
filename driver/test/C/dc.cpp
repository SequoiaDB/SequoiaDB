#include <stdio.h>
#include <gtest/gtest.h>
#include "testcommon.h"
#include "client.h"

TEST(dc, dc_all_api)
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle db     = 0 ;
   sdbCursorHandle cursor     = 0 ;
   sdbDCHandle dc             = 0 ;
#define DC_NAME 255
   CHAR dcName[ DC_NAME + 1 ] =  { 0 } ;
   const CHAR *pPeerCataAddr  = "192.168.20.166:11823" ;
   bson obj ;
   bson arr1 ;
   bson arr2 ;
   bson_init( &obj ) ;
   bson_init( &arr1 ) ;
   bson_init( &arr2 ) ;
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbGetDC( db, &dc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbGetDCName( dc, dcName, DC_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbGetDCDetail( dc, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "Detail is: " ) ;
   bson_print(&obj) ;
   bson_destroy( &obj ) ;

   rc = sdbCreateImage( dc, pPeerCataAddr ) ;
   ASSERT_EQ( SDB_OK, rc ) ;


   bson_init( &obj ) ;
   bson_append_start_array( &obj, "Groups" ) ;

   bson_append_string( &arr1, "0", "group1" ) ;
   bson_append_string( &arr1, "1", "group1" ) ;
   bson_finish( &arr1 ) ;
   bson_append_string( &arr2, "0", "group2" ) ;
   bson_append_string( &arr2, "1", "group2" ) ;
   bson_finish( &arr2 ) ;

   bson_init( &obj ) ;
   bson_append_start_array( &obj, "Groups" ) ;
   bson_append_array( &obj, "0", &arr1 ) ;
   bson_append_array( &obj, "1", &arr2 ) ;
   bson_append_finish_array( &obj ) ;

   rc = bson_finish( &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   printf( "Groups info is: " ) ;
   bson_print( &obj ) ; 

   rc = sdbAttachGroups( dc, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson_destroy( &arr1 ) ;
   bson_destroy( &arr2 ) ;

   rc = sdbEnableReadOnly( dc, FALSE ) ; 
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbEnableReadOnly( dc, TRUE ) ; 
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbEnableImage( dc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbActivateDC( dc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbDeactivateDC( dc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbDisableImage( dc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbEnableReadOnly( dc, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbDetachGroups( dc, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &obj ) ;

   rc = sdbRemoveImage( dc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;


   sdbDisconnect ( db ) ;
   sdbReleaseCursor ( cursor ) ;
   sdbReleaseDC ( dc ) ;
   sdbReleaseConnection ( db ) ;
}

