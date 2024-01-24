/**************************************************************
 * @Description:  test rename cs fail
 *                seqDB-16566
                  seqDB-16567
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

class reNameCSTest16566_16567 : public testBase
{
protected:
   sdbCSHandle csOld ;
   sdbCSHandle csNew ;
   const CHAR* csOldName ;
   const CHAR* csNewName ;

   void SetUp()
   {
      INT32 rc = SDB_OK ;
      csOld = SDB_INVALID_HANDLE ;
      csNew = SDB_INVALID_HANDLE ;
      csOldName = "oldcsname_16566_16567" ;
      csNewName = "newcsname_16566_16567" ;
      testBase::SetUp() ;
   }
   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = sdbDropCollectionSpace( db, csOldName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csOldName ;
         rc = sdbDropCollectionSpace( db, csNewName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csNewName ;
      }
      sdbReleaseCS( csOld ) ;
      sdbReleaseCS( csNew ) ;
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

TEST_F( reNameCSTest16566_16567, renamecsfail )
{
   INT32 rc = SDB_OK ;

   // rename cs and oldCSName is not exist
   bson option ;
   bson_init( &option ) ;
   bson_append_bool( &option, "Global", true ) ;
   bson_finish( &option ) ;
   rc = sdbRenameCollectionSpace( db, csOldName, csNewName, &option ) ;
   ASSERT_EQ( -34, rc ) << "success to rename cs when old cs not exist " << csOldName ;

   // check cs
   BOOLEAN exist ;
   rc = checkCSExist( db, csNewName, &exist ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_FALSE( exist ) << "fail to check rename cs" ;
 
   // create cs
   rc = sdbCreateCollectionSpace( db, csOldName, SDB_PAGESIZE_4K, &csOld ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csOldName ;
   rc = sdbCreateCollectionSpace( db, csNewName, SDB_PAGESIZE_4K, &csNew ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csNewName ;

   // rename cs and newCSName exist
   rc = sdbRenameCollectionSpace( db, csOldName, csNewName, &option ) ;
   ASSERT_EQ( -33, rc ) << "success to rename cs when new cs exist " ; 
   rc = sdbRenameCollectionSpace( db, csNewName, csNewName, &option ) ;
   ASSERT_EQ( -33, rc ) << "success to rename same cs " ;
   
   // check cs
   rc = checkCSExist( db, csOldName, &exist ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( exist ) << "fail to check rename cs" ;
   rc = checkCSExist( db, csNewName, &exist ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( exist ) << "fail to check rename cs" ;

   // destroy bson   
   bson_destroy( &option ) ;
}

