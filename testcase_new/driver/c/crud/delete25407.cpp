/*
 * @Description   : SEQUOIADBMAINSTREAM-7980：sdbDelete1接口测试 seqDB-25407
 * @Author        : xiao zhenfan
 * @CreateTime    : 2022.02.22
 * @LastEditTime  : 2022.02.24
 * @LastEditors   : xiao zhenfan
 */
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"
 class delete25407 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   const CHAR* indexName ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;
   void SetUp() 
   {
      testBase::SetUp() ;  
      INT32 rc = SDB_OK ;
      csName = "cs_25407" ;
      clName = "cl_25407" ;
      cs = 0 ;
      cl = 0 ;
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

TEST_F( delete25407, sdbDelete1 )
{
   INT32 rc = SDB_OK ;
   bson result ;
   bson_init( &result ) ;
   rc = sdbDelete1( cl, NULL, NULL, 0, &result ) ;
   ASSERT_EQ ( SDB_OK, rc ) << "failed to delete ,rc =" << rc ;
   bson_iterator it ;
   ASSERT_EQ( BSON_LONG, bson_find( &it, &result, "DeletedNum" ) ) ;
   ASSERT_EQ( 0, bson_iterator_long( &it ) ) ;
   bson_destroy( &result ) ;
}
