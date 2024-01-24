/**************************************************************
 * @Description:  test rename CL fail
 *                seqDB-16570
 *                seqDB-16571
 * @Author     :  liuxiaoxuan Init
 * @Date       :  2018-11-29
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "testBase.hpp"
#include "testcommon.hpp"
#include "arguments.hpp"

class reNameCLTest16570_16571 : public testBase
{
protected:
   sdbCSHandle cs ;
   sdbCollectionHandle clOld ;
   sdbCollectionHandle clNew ;
   const CHAR* csName ;
   const CHAR* clOldName ;
   const CHAR* clNewName ;

   void SetUp()
   {
      INT32 rc = SDB_OK ;
      cs = SDB_INVALID_HANDLE ;
      clOld = SDB_INVALID_HANDLE ;
      clNew = SDB_INVALID_HANDLE ;
      csName = "csname_16570_16571";
      clOldName = "oldclname_16570_16571" ;
      clNewName = "newclname_16570_16571" ;
      testBase::SetUp() ;
   }
   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = sdbDropCollectionSpace( db, csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      }
      sdbReleaseCS( cs ) ;
      sdbReleaseCollection( clOld ) ;
      sdbReleaseCollection( clNew ) ;
      testBase::TearDown() ;
   }

};

INT32 checkCLExist( sdbConnectionHandle db, const CHAR* clFullName, BOOLEAN* exist )
{
   INT32 rc = SDB_OK ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson obj ;
   bson_init( &obj ) ;
   *exist = FALSE ;

   rc = sdbListCollections( db, &cursor ) ;
   CHECK_RC( SDB_OK, rc, "fail to list cs" ) ;

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

TEST_F( reNameCLTest16570_16571, renameclfail )
{
   INT32 rc = SDB_OK ;

   // create cs
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;

   // rename cl when old cl not exist
   bson option ;
   bson_init( &option ) ;
   bson_append_bool( &option, "Global", true ) ;
   bson_finish( &option ) ;
   rc = sdbRenameCollection( cs, clOldName, clNewName, &option ) ;
   ASSERT_EQ( -23, rc ) << "success to rename old cl: " << clOldName << " new cl: " << clNewName ;

   // check cl
   CHAR clNewFullName[100] ;
   sprintf( clNewFullName, "%s.%s", csName, clNewName ) ;
   BOOLEAN exist ;
   rc = checkCLExist( db, clNewFullName, &exist ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_FALSE( exist ) << "fail to check rename cl" ;

   // create cl
   rc = sdbCreateCollection( cs, clOldName, &clOld ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clOldName ;
   rc = sdbCreateCollection( cs, clNewName, &clNew ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clNewName ;

   // rename cl when new cl exist
   rc = sdbRenameCollection( cs, clOldName, clNewName, &option ) ;
   ASSERT_EQ( -22, rc ) << "fail to rename cl when new cl exist " ;
   rc = sdbRenameCollection( cs, clNewName, clNewName, &option ) ;
   ASSERT_EQ( -22, rc ) << "fail to rename same cl " ;
   
   rc = checkCLExist( db, clNewFullName, &exist ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( exist ) << "fail to check rename cl" ;
   CHAR clOldFullName[100] ;
   sprintf( clOldFullName, "%s.%s", csName, clOldName ) ;
   rc = checkCLExist( db, clOldFullName, &exist ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( exist ) << "fail to check rename cl" ;

   // destroy bson
   bson_destroy( &option ) ;
}
