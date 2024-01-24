/**************************************************************************
 * @Description:   seqDB-13414: 未seek写lob
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

class lobWrite13414 : public testBase
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
      pCsName = "lobWrite13414" ;
      pClName = "lobWrite13414" ;

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

TEST_F( lobWrite13414, lobWrite )
{
   INT32 rc                 = SDB_OK ;
   sdbLob lob ;
   OID oid ;
   const CHAR *bigBuf       = "0123456789ABCDEabcde" ;
   const INT32 bigBufSize   = strlen( bigBuf ) ;
   const CHAR *smallBuf     = "9876543210" ;
   const INT32 smallBufSize = strlen( smallBuf ) ;
   const UINT32 readBufSize = bigBufSize ;
   CHAR readBuf[ readBufSize ] ;
   UINT32 read ;
   CHAR expBuf[ readBufSize ] ;

   // create and write lob
   rc = cl.createLob( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;
   rc = lob.write( bigBuf, bigBufSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;

   // close and open again
   rc = lob.getOid( oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get oid" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
   rc = cl.openLob( lob, oid, SDB_LOB_WRITE ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;

   // write without seek
   rc = lob.write( smallBuf, smallBufSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
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
   memcpy( expBuf, bigBuf, bigBufSize ) ;
   memcpy( expBuf, smallBuf, smallBufSize ) ;
   ASSERT_EQ( 0, memcmp( expBuf, readBuf, bigBufSize ) ) << "wrong lob content" ;
}
