/**************************************************************
 * @Description:  test rename cs
 *                seqDB-16565
 * @Author     :  liuxiaoxuan Init
 * @Date       :  2018-11-28
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "testBase.hpp"
#include "testcommon.hpp"
#include "arguments.hpp"

class reNameCSTest16565 : public testBase
{
protected:
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   const CHAR* csOldName ;
   const CHAR* csNewName ;

   void SetUp()
   {
      INT32 rc = SDB_OK ;
      cs = SDB_INVALID_HANDLE ;
      cl = SDB_INVALID_HANDLE ;
      csOldName = "oldcsname_16565" ;
      csNewName = "newcsname_16565" ;
      testBase::SetUp() ;
   }
   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = sdbDropCollectionSpace( db, csNewName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csNewName ;
      }
      sdbReleaseCS( cs ) ;
      sdbReleaseCollection( cl ) ;
      testBase::TearDown() ;
   }

};

INT32 checkCSExist( sdbConnectionHandle db, const CHAR* csName, BOOLEAN* exist )
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

TEST_F( reNameCSTest16565, renamecs )
{
   INT32 rc = SDB_OK ;

   // create cs
   rc = sdbCreateCollectionSpace( db, csOldName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csOldName ;

   // rename cs with new name
   bson option ;
   bson_init( &option ) ;
   bson_append_bool( &option, "Global", true ) ;
   bson_finish( &option ) ;
   rc = sdbRenameCollectionSpace( db, csOldName, csNewName, &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to rename cs oldName: " << csOldName << " newName: " << csNewName ; 
   bson_destroy( &option ) ;

   // check rename cs
   BOOLEAN exist ;
   rc = checkCSExist( db, csOldName, &exist ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_FALSE( exist ) << "fail to check rename cs" ;
   rc = checkCSExist( db, csNewName, &exist ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( exist ) << "fail to check rename cs" ;

   // create cl with new cs
   sdbReleaseCS( cs ) ;
   rc = sdbGetCollectionSpace( db, csNewName, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs" << csNewName ;
   const CHAR* clName = "clname_16565" ;
   rc = sdbCreateCollection( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

   // drop cl with new cs
   rc = sdbDropCollection( cs, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
}

