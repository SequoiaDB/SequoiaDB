/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-11063:执行closeAllCursors后执行lob操作
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

class closeAllCursorsTest11063 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "lobCloseTestCs12554" ;
      clName = "lobCloseTestCl12554" ;
      rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;
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

TEST_F( closeAllCursorsTest11063, closeAllCursorsAfterCreate11063 )
{
   INT32 rc = SDB_OK ;

   // closeAllCursors after createLob
   sdbLob lob ;
   rc = cl.createLob( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to createLob" ;
   ASSERT_FALSE( lob.isClosed() ) << "fail to check lob isClosed" ;
   rc = db.closeAllCursors() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to closeAllCursors" ;
   ASSERT_TRUE( lob.isClosed() ) << "fail to check lob isClosed" ;   

   const CHAR* buf = "0123456789ABCDEFabcdef" ;
   UINT32 bufLen = strlen( buf ) ; 
   rc = lob.write( buf, bufLen ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) << "fail to test write after closeAllCursors" ;
   OID oid = lob.getOid() ;
   SINT64 lobSize = lob.getSize() ;
   UINT64 time = lob.getCreateTime() ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close after closeAllCursors" ;
}

TEST_F( closeAllCursorsTest11063, closeAllCursorsAfterOpen11063 )
{
   INT32 rc = SDB_OK ;

   sdbLob lob ;
   rc = cl.createLob( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to createLob" ;
   const CHAR* buf = "0123456789ABCDEFabcdef" ;
   UINT32 bufLen = strlen( buf ) ;
   rc = lob.write( buf, bufLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write" ;
   OID oid = lob.getOid() ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
   
   // closeAllCursors after open lob
   rc = cl.openLob( lob, oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob" ;
   rc = db.closeAllCursors() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to closeAllCursors" ;
   
   UINT32 len = 10 ;
   CHAR readBuf[100] = { 0 } ;
   UINT32 readLen ;
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) << "fail to test read after closeAllCursors" ;
   SINT64 offset = 0 ;
   SDB_LOB_SEEK whence = SDB_LOB_SEEK_SET ;
   rc = lob.seek( offset, whence ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) << "fail to test seek after closeAllCursors" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test close after closeAllCursors" ;

   // closeAllCursors after closeAllCursors
   rc = db.closeAllCursors() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test closeAllCursors after closeAllCursors" ;
}
