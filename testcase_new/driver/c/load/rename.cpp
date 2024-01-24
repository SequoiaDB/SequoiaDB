/**************************************************************
 * @Description: test case for Jira questionaire Task
 *				     SEQUOIADBMAINSTREAM-2165
 *				     seqDB-10999:renameCS
 *				     seqDB-11000:renameCL
 * @Modify     : Liang xuewang Init
 *			 	     2017-01-22
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "testcommon.hpp"
#include "arguments.hpp"

sdbConnectionHandle db   = SDB_INVALID_HANDLE ;
sdbCSHandle cs           = SDB_INVALID_HANDLE ;
sdbCollectionHandle cl   = SDB_INVALID_HANDLE ;
const CHAR* csOldName    = "renameCSTestCs1" ;
const CHAR* csNewName    = "renameCSTestCs2" ;
const CHAR* clOldName    = "renameCLTestCl1" ;
const CHAR* clNewName    = "renameCLTestCl2" ;

INT32 checkCsExist( sdbConnectionHandle db, const CHAR* csName, BOOLEAN* exist )
{
   INT32 rc = SDB_OK ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson obj ;
   bson_init( &obj ) ;
   *exist = FALSE ;

   rc = sdbListCollectionSpaces( db, &cursor ) ;
   CHECK_RC( SDB_OK, rc, "fail to list cs" ) ;

   while( !sdbNext( cursor, &obj ) )
   {
      bson_iterator it ;
      bson_iterator_init( &it, &obj ) ;
      const CHAR* name = bson_iterator_string( &it ) ;
      if( !strcmp( name, csName ) )
      {
         *exist = TRUE ;
         break ;
      }
      bson_destroy( &obj ) ;
      bson_init( &obj ) ;
   }

done:
   bson_destroy( &obj ) ;
   sdbReleaseCursor( cursor ) ;
   return rc ;
error:
   goto done ;
}

INT32 checkClExist( sdbConnectionHandle db, const CHAR* clFullName, BOOLEAN* exist )
{
   INT32 rc = SDB_OK ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson obj ;
   bson_init( &obj ) ;
   *exist = FALSE ;

   rc = sdbListCollections( db, &cursor ) ;
   CHECK_RC( SDB_OK, rc, "fail to list cl" ) ;

   while( !sdbNext( cursor, &obj ) )
   {
      bson_iterator it ;
      bson_iterator_init( &it, &obj ) ;
      const CHAR* name = bson_iterator_string( &it ) ;
      if( !strcmp( name, clFullName ) )
      {
         *exist = TRUE ;
         break ;
      }
      bson_destroy( &obj ) ;
      bson_init( &obj ) ;
   }

done:
   bson_destroy( &obj ) ;
   sdbReleaseCursor( cursor ) ;
   return rc ;
error:
   goto done ;
}

INT32 checkBasicOperation( sdbCollectionHandle cl )
{
   INT32 rc = SDB_OK ;

   bson record, selector ;
   bson_init( &record ) ;
   bson_init( &selector ) ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;

   // insert
   bson_append_int( &record, "a", 1 ) ;
   bson_finish( &record ) ;
   rc = sdbInsert( cl, &record ) ;
   CHECK_RC( SDB_OK, rc, "fail to insert record" ) ;

   // query
   bson_append_string( &selector, "a", "" ) ;
   bson_finish( &selector ) ;
   rc = sdbQuery( cl, &record, &selector, NULL, NULL, 0, -1, &cursor ) ;
   CHECK_RC( SDB_OK, rc, "fail to query record" ) ;
   rc = sdbCloseCursor( cursor ) ;
   CHECK_RC( SDB_OK, rc, "fail to close cursor" ) ;

done:
   bson_destroy( &record ) ;
   bson_destroy( &selector ) ;
   sdbReleaseCursor( cursor ) ;
   return rc ; 
error:
   goto done ;
}

TEST( rename, renameCS )
{
   INT32 rc = SDB_OK ;

   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect" ;
   if( !isStandalone( db ) )
   {
      sdbDisconnect( db ) ;
      sdbReleaseConnection( db ) ;
      return ;
   }

   // create cs with old name
   rc = sdbCreateCollectionSpace( db, csOldName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csOldName ;

   // rename cs with new name
   bson option ;
   bson_init( &option ) ;
   bson_append_bool( &option, "Global", true ) ;
   bson_finish( &option ) ;
   rc = sdbRenameCollectionSpace( db, csOldName, csNewName, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to rename cs oldName: " << csOldName << " newName: " << csNewName ; 

   // check rename cs
   BOOLEAN exist ;
   rc = checkCsExist( db, csOldName, &exist ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_FALSE( exist ) << "fail to check rename cs" ;
   rc = checkCsExist( db, csNewName, &exist ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( exist ) << "fail to check rename cs" ;

   // create cl with cs
   sdbReleaseCS( cs ) ;
   rc = sdbGetCollectionSpace( db, csNewName, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs" << csNewName ;
   rc = sdbCreateCollection( cs, clOldName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clOldName ;

   // rename cl with new name
   rc = sdbRenameCollection( cs, clOldName, clNewName, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to rename cl olName: " << clOldName << " newName: " << clNewName ;

   // check rename cl
   CHAR clOldFullName[200] ;
   sprintf( clOldFullName, "%s.%s", csNewName, clOldName ) ;
   CHAR clNewFullName[200] ;
   sprintf( clNewFullName, "%s.%s", csNewName, clNewName ) ;
   rc = checkClExist( db, clOldFullName, &exist ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_FALSE( exist ) << "fail to check rename cl" ;
   rc = checkClExist( db, clNewFullName, &exist ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( exist ) << "fail to check rename cl" ;

   // check basic operation with new name cl
   sdbReleaseCollection( cl ) ;
   rc = sdbGetCollection1( cs, clNewName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl " << clNewName ;
   rc = checkBasicOperation( cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbDropCollectionSpace( db, csNewName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csNewName ;
   sdbDisconnect( db ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection( db ) ;	
}
