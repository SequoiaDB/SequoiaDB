/**************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-2504
 * @Modify:      Liang xuewang Init
 *			 	     2017-06-19
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class prepareMoreTest : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;

   void SetUp() 
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "prepareMoreTestCs" ;
      clName = "prepareMoreTestCl" ;
      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollectionSpace( db, csName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      testBase::TearDown() ;
   }
} ;

TEST_F( prepareMoreTest, prepareMore )
{
   INT32 rc = SDB_OK ;

   // insert doc
   bson doc ;
   bson_init( &doc ) ;
   rc = bson_append_int( &doc, "a", 1 ) ;
   ASSERT_EQ( BSON_OK, rc ) << "fail to append bson a:1" ;
   rc = bson_finish( &doc ) ;
   ASSERT_EQ( BSON_OK, rc ) << "fail to finish bson" ;
   rc = sdbInsert( cl, &doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   // query doc 
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   rc = sdbQuery1( cl, &doc, NULL, NULL, NULL, 0,
                   -1, QUERY_PREPARE_MORE, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   bson obj ;
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   bson_iterator it ;
   bson_find( &it, &obj, "a" ) ;
   INT32 value = bson_iterator_int( &it ) ;
   ASSERT_EQ( 1, value ) << "fail to check query result" ;

   // release resource
   bson_destroy( &obj ) ;
   bson_destroy( &doc ) ;
   sdbReleaseCursor( cursor ) ;
}
