/**************************************************
 * @Description: test case for c++ driver
 *               seqDB-15382: use invalid flag to query
 * @Modify:      Suqiang Ling
 *               2018-06-01
 **************************************************/
#include <client.h>
#include <gtest/gtest.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class invalidFlag15382 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   INT32 docNum ;

   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "invalidFlag15382" ;
      clName = "invalidFlag15382" ;
      INT32 i ;
      docNum = 20 ;

      rc = createNormalCsCl( db, &cs, &cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ; 
      bson* docs[docNum] ;
      for( i = 0; i < docNum; ++i )
      {
         docs[i] = bson_create() ;
         bson_append_int( docs[i], "a", 1 ) ;
         bson_finish( docs[i] ) ;
      }
      rc = sdbBulkInsert( cl, 0, docs, docNum ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to insert documents" ;
      for( i = 0; i < docNum; ++i )
      {
         bson_dispose( docs[i] ) ;
      }
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = SDB_OK ;
         rc = sdbDropCollectionSpace( db, csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
         sdbReleaseCollection( cl ) ;
         sdbReleaseCS( cs ) ;
      }
      testBase::TearDown() ;
   }
} ;

TEST_F( invalidFlag15382, test )
{
	INT32 rc = SDB_OK ;
   sdbCursorHandle cursor ;
   // dft: short of default
	bson *dftCond     = NULL ;
	bson *dftSelect   = NULL ;
	bson *dftOrder    = NULL ;
	bson *dftHint     = NULL ;
   INT64 dftSkipNum  = 0 ;
   INT64 dftReturnNum= -1 ;

   INT32 invalidFlag = 4096;
   bson *update = bson_create() ;
   bson_append_start_object( update, "$set" ) ;
   bson_append_int( update, "a", 1 ) ;
   bson_append_finish_object( update ) ;
   bson_finish( update ) ;


   rc = sdbQuery1( cl, dftCond, dftSelect, dftOrder, dftHint, dftSkipNum, dftReturnNum, invalidFlag, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbQueryAndUpdate( cl, dftCond, dftSelect, dftOrder, dftHint, update,
                           dftSkipNum, dftReturnNum, invalidFlag, false, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ; // force hint flag is on

   rc = sdbQueryAndRemove( cl, dftCond, dftSelect, dftOrder, dftHint, 
                           dftSkipNum, dftReturnNum, invalidFlag, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ; // force hint flag is on
}
