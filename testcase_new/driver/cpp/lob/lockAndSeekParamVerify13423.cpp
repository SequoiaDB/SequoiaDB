/**************************************************************************
 * @Description:   seqDB-13423: lockAndSeek接口参数校验
 * @Modify:        Suqiang Ling
 *                 2017-11-17
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

class lockAndSeekParamVerify13423 : public testBase
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   
   void SetUp()
   {
      testBase::SetUp() ;

      INT32 rc = SDB_OK ;
      pCsName = "lockAndSeekParamVerify13423" ;
      pClName = "lockAndSeekParamVerify13423" ;

      // create cs, cl
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = cs.createCollection( pClName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = db.dropCollectionSpace( pCsName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << pCsName ;
      } 
      testBase::TearDown() ;
   }
} ;

TEST_F( lockAndSeekParamVerify13423, lockLob )
{
   INT32 rc = SDB_OK ;
   sdbLob lob ;
   OID oid ;
   const CHAR* buf = "1234567890" ;
   const INT64 bufSize = strlen( buf ) ;
   const INT64 maxInt64 = 0x7FFFFFFFFFFFFFFFLL ;
   const INT64 minInt64 = -9223372036854775807LL-1 ;
   const INT64 normalVal = 8 ;

   // prepare a lob
   rc = cl.createLob( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;
   rc = lob.getOid( oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get oid" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
   rc = cl.openLob( lob, oid, SDB_LOB_WRITE ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;

   // illegal parameter
   rc = lob.lockAndSeek( -1, normalVal ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "rc must be SDB_INVALIDARG, when offset is -1" ;
   rc = lob.lockAndSeek( minInt64, normalVal ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "rc must be SDB_INVALIDARG, when offset is minInt64" ;
   rc = lob.lockAndSeek( normalVal, -2 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "rc must be SDB_INVALIDARG, when length is -2" ;
   rc = lob.lockAndSeek( normalVal, 0 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "rc must be SDB_INVALIDARG, when length is 0" ;
   rc = lob.lockAndSeek( normalVal, minInt64 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "rc must be SDB_INVALIDARG, when length is minInt64" ;
   rc = lob.lockAndSeek( maxInt64 / 2 + 1, maxInt64 / 2 + 1 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "rc must be SDB_INVALIDARG, when (offset + length) > maxInt64" ;

   // legal parameter
   rc = lob.lockAndSeek( 0, normalVal ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lockAndSeek with offset 0" ;
   rc = lob.lockAndSeek( maxInt64 - 1, 1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lockAndSeek with offset maxInt64" ;
   rc = lob.lockAndSeek( normalVal, -1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lockAndSeek with length -1" ;
   rc = lob.lockAndSeek( 0, maxInt64 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lockAndSeek with length maxInt64" ;

   // check legal lock usable
   rc = lob.seek( 0, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek lob" ;
   rc = lob.write( buf, bufSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}
