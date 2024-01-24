/**************************************************************************
 * @Description:   seqDB-13420: 读取lob中空切片数据
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

class readEmptyPiece13420 : public testBase
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
      pCsName = "readEmptyPiece13420" ;
      pClName = "readEmptyPiece13420" ;
      BSONObj csOpt ;

      // create cs, cl
      csOpt = BSON( "LobPageSize" << SDB_PAGESIZE_4K ) ;
      rc = db.createCollectionSpace( pCsName, csOpt, cs ) ; 
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

TEST_F( readEmptyPiece13420, readLob )
{
   INT32 rc                = SDB_OK ;
   sdbLob lob ;
   OID oid ;
   const CHAR *buf1        = "0123456789ABCDEabcde" ;
   const INT32 bufSize1    = strlen( buf1 ) ;
   const INT32 emptyOffset = bufSize1 ;
   const INT32 emptyLength = 12 ;
   const CHAR *buf2        = "9876543210" ;
   const INT32 bufSize2    = strlen( buf2 ) ;
   const INT32 readBufSize = bufSize1 + emptyLength + bufSize2 ;
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

   // lock and write discontinuous ranges
   // [ buf1 ][ empty ][ buf2 ]
   rc = cl.openLob( lob, oid, SDB_LOB_WRITE ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;
   rc = lob.lockAndSeek( 0, bufSize1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lock lob" ;
   rc = lob.write( buf1, bufSize1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = lob.lockAndSeek( emptyOffset + emptyLength, bufSize2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lock lob" ;
   rc = lob.write( buf2, bufSize2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // read only empty range
   rc = cl.openLob( lob, oid, SDB_LOB_READ ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;
   rc = lob.seek( emptyOffset, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek lob" ;
   rc = lob.read( emptyLength, readBuf, &read ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( emptyLength, read ) << "long read length is wrong" ;
   memset( expBuf, 0, emptyLength ) ;
   ASSERT_EQ( 0, memcmp( expBuf, readBuf, emptyLength ) ) << "wrong empty piece" ;

   // read content that contains empty pieces
   rc = lob.seek( 0, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seek lob" ;
   rc = lob.read( readBufSize, readBuf, &read ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( readBufSize, read ) << "long read length is wrong" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   memcpy( expBuf, buf1, bufSize1 ) ;
   memset( expBuf + bufSize1, 0, emptyLength ) ;
   memcpy( expBuf + bufSize1 + emptyLength, buf2, bufSize2 ) ;
   ASSERT_EQ( 0, memcmp( expBuf, readBuf, readBufSize ) ) << "wrong lob content" ;
}
