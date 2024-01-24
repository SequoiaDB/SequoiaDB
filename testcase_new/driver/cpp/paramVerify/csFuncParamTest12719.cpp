/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12719:getCollection参数校验
 *                 seqDB-12720:createCollection参数校验
 *                 seqDB-12721:dropCollection参数校验
 *                 seqDB-14691:renameCollection参数校验
 * @Modify:        Liang xuewang Init
 *                 2017-08-29
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class csFuncParamTest12719 : public testBase
{
protected:
   const CHAR* csName ;
   sdbCollectionSpace cs ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "csFuncParamTestCs12635" ;
      rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = db.dropCollectionSpace( csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      } 
      testBase::TearDown() ;
   }
} ;

TEST_F( csFuncParamTest12719, getCollection12719 )
{
   INT32 rc = SDB_OK ;

   string clLongName( 128, 'x' ) ;
   const char* clName = clLongName.c_str() ;
   sdbCollection cl ;
   rc = cs.getCollection( clName, cl ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to get cl with long name" ;

   rc = cs.getCollection( NULL, cl ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to get cl with NULL" ;
}

TEST_F( csFuncParamTest12719, createCollection12720 )
{
   INT32 rc = SDB_OK ;

   string clLongName( 128, 'x' ) ;
   const char* clName = clLongName.c_str() ;
   BSONObj option = BSON( "ShardingKey" << BSON( "a" << 1 ) << "ShardingType" << "hash" ) ;
   sdbCollection cl ;
   rc = cs.createCollection( clName, option, cl ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to get cl with long name" ;

   rc = cs.createCollection( NULL, option, cl ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to get cl with NULL" ;
}

TEST_F( csFuncParamTest12719, dropCollection12721 )
{
   INT32 rc = SDB_OK ;

   string clLongName( 128, 'x' ) ;
   const char* clName = clLongName.c_str() ;
   rc = cs.dropCollection( clName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to get cl with long name" ;

   rc = cs.dropCollection( NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to get cl with NULL" ;
}

TEST_F( csFuncParamTest12719, renameCollection14691 )
{
   INT32 rc =  SDB_OK ;

   const CHAR* oldName = "testCl14691" ;
   const CHAR* newName = "testCl14691_1" ;
   rc = cs.renameCollection( NULL, newName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = cs.renameCollection( oldName, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}
