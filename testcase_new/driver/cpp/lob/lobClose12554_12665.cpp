/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12554:lob关闭后执行读写
 *                 seqDB-12665:重复关闭lob
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

class lobCloseTest12554 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbLob lob ;
   INT32 lobSize ;
   UINT64 time ;
   OID oid ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "lobCloseTestCs12554" ;
      clName = "lobCloseTestCl12554" ;
      rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;

      rc = cl.createLob( lob ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;
      const CHAR* buf = "0123456789ABCDEFabcdef" ;
      lobSize = strlen( buf ) ;
      rc = lob.write( buf, lobSize ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
      oid = lob.getOid() ;
      time = lob.getCreateTime() ;
      rc = lob.close() ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
      ASSERT_TRUE( lob.isClosed() ) << "fail to check lob isClosed" ;
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

TEST_F( lobCloseTest12554, rwAfterClose12554 )
{
   INT32 rc = SDB_OK ;

   const CHAR* buf = "lxw" ;
   UINT32 bufLen = strlen( buf ) ;
   rc = lob.write( buf, bufLen ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) << "fail to check write after lob close" ;

   UINT32 len = 5 ;
   CHAR readBuf[10] ;
   UINT32 readLen ;
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) << "fail to check read after lob close" ;

   SINT64 offset = 0 ;
   SDB_LOB_SEEK whence = SDB_LOB_SEEK_SET ;
   rc = lob.seek( offset, whence ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) << "fail to check seek after lob close" ;

   ASSERT_EQ( lobSize, lob.getSize() ) << "fail to check get lobSize after close" ;
   ASSERT_EQ( oid, lob.getOid() ) << "fail to check getOid after close" ;
   ASSERT_EQ( time, lob.getCreateTime() ) << "fail to check getCreateTime after close" ;
}

TEST_F( lobCloseTest12554, closeAfterClose12665 )
{
   INT32 rc = SDB_OK ;

   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test close after close" ;
}
