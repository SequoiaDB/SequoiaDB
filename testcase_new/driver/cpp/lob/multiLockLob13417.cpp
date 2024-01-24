/**************************************************************************
 * @Description:   seqDB-13417: 锁定多个数据范围后写入lob
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

class multiLockLob13417 : public testBase
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
      pCsName = "multiLockLob13417" ;
      pClName = "multiLockLob13417" ;

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

TEST_F( multiLockLob13417, lockLob )
{
   INT32 rc                = SDB_OK ;
   sdbLob lob ;
   OID oid ;
   const CHAR *buf1        = "0123456789ABCDEabcde" ;
   const INT32 bufSize1    = strlen( buf1 ) ;
   const CHAR *buf2        = "9876543210" ;
   const INT32 bufSize2    = strlen( buf2 ) ;
   const INT32 readBufSize = bufSize1 + bufSize2 ;
   CHAR readBuf[ readBufSize ] ;
   UINT32 read ;
   CHAR expBuf[ readBufSize ] ;

   // create a lob
   rc = cl.createLob( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;
   rc = lob.getOid( oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get oid" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // lock, and then seek
   rc = cl.openLob( lob, oid, SDB_LOB_WRITE ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;
   rc = lob.lock( 0, bufSize1 / 2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lock lob" ;
   rc = lob.lock( bufSize1 / 2, bufSize1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lock lob" ;
   rc = lob.seek( 0, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek lob" ;
   rc = lob.write( buf1, bufSize1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;

   // lock and seek
   rc = lob.lockAndSeek( bufSize1, bufSize2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lock and seek lob" ;
   rc = lob.write( buf2, bufSize2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // read lob 
   rc = cl.openLob( lob, oid, SDB_LOB_READ ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;
   rc = lob.read( readBufSize, readBuf, &read ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( readBufSize, read ) << "lob read length is wrong" ;
   ASSERT_EQ( readBufSize, lob.getSize() ) << "wrong lob size" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // check lob
   memcpy( expBuf, buf1, bufSize1 ) ;
   memcpy( expBuf + bufSize1, buf2, bufSize2 ) ;
   ASSERT_EQ( 0, memcmp( expBuf, readBuf, readBufSize ) ) << "wrong lob content" ;
}
