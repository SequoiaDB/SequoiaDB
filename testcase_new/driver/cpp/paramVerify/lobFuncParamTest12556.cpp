/**************************************************************
 * @Description: parameter verification for class sdbLob
 *               seqDB-12556 : read/write lob length zero
 *               seqDB-12734 : read() parameter verification
 *               seqDB-12556 : write() parameter verification
 *               seqDB-14364:getSize/getCreateTime参数校验
 * @Modify     : Suqiang Ling
 *               2017-09-11
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include <vector>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class lobFuncParamTest : public testBase 
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;
   sdbCollection cl ;

   void SetUp() 
   {
      testBase::SetUp() ;

      pCsName = "lobParam12556" ;
      pClName = "lobParam12556" ;
      sdbCollectionSpace cs ;

      INT32 rc = SDB_OK ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = cs.createCollection( pClName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;
   }

   void TearDown() 
   {
      if( shouldClear() )
      {
         INT32 rc = SDB_OK ;
         rc = db.dropCollectionSpace( pCsName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
      }
      testBase::TearDown() ;
   }
} ;

TEST_F( lobFuncParamTest, lengthZero12556 )
{
   INT32 rc = SDB_OK ;
   sdbLob wlob ;
   const CHAR *wBuf = "" ;
   SINT64 size ; 

   rc = cl.createLob( wlob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;
   rc = wlob.write( wBuf, 0 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = wlob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   rc = wlob.getSize( &size ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get lob size" ;
   ASSERT_EQ( 0, size ) << "written lob size is wrong" ;

   OID oid ;
   rc = wlob.getOid( oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get oid" ;

   sdbLob rlob ;
   CHAR rBuf[8] = { 0 } ;
   UINT32 read ;

   rc = cl.openLob( rlob, oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;
   rc = rlob.read( 0, rBuf, &read ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   rc = rlob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
   ASSERT_EQ( 0, read ) << "read lob size is wrong" ;
}

TEST_F( lobFuncParamTest, read12734 )
{
   INT32 rc = SDB_OK ;
   sdbLob wlob ;
   const CHAR *wBuf = "123456789" ;
   SINT64 size ; 

   rc = cl.createLob( wlob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;
   rc = wlob.write( wBuf, 10 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = wlob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   OID oid ;
   rc = wlob.getOid( oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get oid" ;

   sdbLob rlob ;
   CHAR *rBuf ;
   UINT32 read ;

   rc = cl.openLob( rlob, oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;
   rc = rlob.read( 10, NULL, &read ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = rlob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}

TEST_F( lobFuncParamTest, write12735 )
{
   INT32 rc = SDB_OK ;
   sdbLob wlob ;

   rc = cl.createLob( wlob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;
   rc = wlob.write( NULL, 10 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = wlob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}

TEST_F( lobFuncParamTest, null14364 )
{
   INT32 rc = SDB_OK ;
   sdbLob wlob ;
   
   rc = cl.createLob( wlob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;
   rc = wlob.getSize( NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = wlob.getCreateTime( NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = wlob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}
