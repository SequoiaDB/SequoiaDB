/**************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-2131
 * @Modify:      Liang xuewang Init
 *			 	     2017-01-07
 **************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class codeTest : public testBase
{
protected:
   void TestUp()
   {
      testBase::SetUp() ;
   }
   void TearDown()
   {
      testBase::TearDown() ;
   }
} ;

TEST_F( codeTest, code )
{
   INT32 rc = SDB_OK ;

   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone,cannot create procedure.\n" ) ;
      return ;
   }

   // create procedure
   const CHAR* function = "function add(a,b) { return a+b ; }" ;
   rc = sdbCrtJSProcedure( db, function ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create procedure" ;

   // list procedure
   bson condition ;
   bson_init( &condition ) ;
   bson_append_string( &condition, "name", "add" ) ;
   bson_finish( &condition ) ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   rc = sdbListProcedures( db, &condition, &cursor ) ;
   bson_destroy( &condition ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to list procedures" ;

   // check procedure
   bson procedure ;
   bson_init( &procedure ) ;
   rc = sdbNext( cursor, &procedure ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next in cursor" ;
   bson_iterator it ;
   bson_type type = bson_find( &it, &procedure, "func" ) ;
   ASSERT_EQ( BSON_CODE, type ) << "fail to check bson type" ;
   const CHAR* code = bson_iterator_code( &it ) ;
   ASSERT_STREQ( function, code ) ;  
   bson_destroy( &procedure ) ;
   sdbReleaseCursor( cursor ) ;

   // remove procedure
   rc = sdbRmProcedure( db, "add" ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to remove procedure" ;
}
