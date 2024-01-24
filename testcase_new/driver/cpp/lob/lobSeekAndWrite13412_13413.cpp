/**************************************************************************
 * @Description:   seqDB-13412: seek偏移写lob
 *                 seqDB-13413: write模式下未加锁写lob 
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

class lobSeekAndWrite13412_13413 : public testBase
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
      pCsName = "lobSeekAndWrite13412" ;
      pClName = "lobSeekAndWrite13412" ;

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

   INT32 seekAndWriteLob( sdbLob &lob, const SINT64 offset, const CHAR *buf )
   {
      INT32 rc = SDB_OK ;
      INT32 bufSize ;

      // seq-13413: don't call lob.lock() here
      rc = lob.seek( offset, SDB_LOB_SEEK_SET ) ;
      CHECK_RC( SDB_OK, rc, "%s", "fail to seek lob" ) ;

      bufSize = strlen( buf ) ;
      rc = lob.write( buf, bufSize ) ;
      CHECK_RC( SDB_OK, rc, "%s", "fail to write lob" ) ;
   done:
      return rc ;
   error:
      goto done ;
   }
} ;

TEST_F( lobSeekAndWrite13412_13413, lobSeekAndWrite )
{
   INT32 rc                 = SDB_OK ;
   sdbLob lob ;
   OID oid ;
   SINT64 offset ;
   const CHAR *writeBuf     = "0123456789ABCDEabcde" ;
   const INT32 wBufSize     = strlen( writeBuf ) ;
   const UINT32 readBufSize = 2 * wBufSize ;
   CHAR readBuf[ readBufSize ] ;
   UINT32 read ;
   CHAR expBuf[ readBufSize ] ;

   // create lob
   rc = cl.createLob( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;
   
   // seek and write
   offset = 0 ;
   rc = seekAndWriteLob( lob, offset, writeBuf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seekAndWriteLob(1)" ;

   // close and open again
   rc = lob.getOid( oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get oid" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
   rc = cl.openLob( lob, oid, SDB_LOB_WRITE ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;

   // seek and write again
   offset = wBufSize ;
   rc = seekAndWriteLob( lob, offset, writeBuf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to seekAndWriteLob(2)" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // read lob 
   rc = cl.openLob( lob, oid, SDB_LOB_READ ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;
   rc = lob.read( readBufSize, readBuf, &read ) ;
   ASSERT_EQ( readBufSize, read ) << "long read length is wrong" ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // check lob content
   memcpy( expBuf, writeBuf, wBufSize ) ;
   memcpy( expBuf + wBufSize, writeBuf, wBufSize ) ;
   ASSERT_EQ( 0, memcmp( expBuf, readBuf, readBufSize ) ) << "wrong lob content" ;
}
