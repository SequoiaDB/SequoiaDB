/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12737:创建lob后读lob偏移lob
 *                 seqDB-12738:打开lob后写lob
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

class lobModeTest12737 : public testBase
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
      csName = "lobModeTestCs12737" ;
      clName = "lobModeTestCl12737" ;
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

TEST_F( lobModeTest12737, createMode12737 )
{
   INT32 rc = SDB_OK ;

   sdbLob lob ;
   rc = cl.createLob( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to createLob" ;

   UINT32 len = 10 ;
   CHAR readBuf[100] = { 0 } ;
   UINT32 readLen ;
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test read with createMode" ;
   
   rc = lob.seek( 0, SDB_LOB_SEEK_SET ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to test seek with createMode" ;

   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}

TEST_F( lobModeTest12737, openMode12738 )
{
   INT32 rc = SDB_OK ;

   // create lob first
   sdbLob lob ;
   rc = cl.createLob( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to createLob" ;
   const CHAR* buf = "0123456789ABCDEFabcdef" ;
   UINT32 bufLen = strlen( buf ) ;
   rc = lob.write( buf, bufLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = lob.write( buf, bufLen ) ;          
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob again" ;
   ASSERT_EQ( 2*bufLen, lob.getSize() ) << "fail to check lobSize" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
   OID oid = lob.getOid() ;

   // test write with openMode
   rc = cl.openLob( lob, oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to openLob" ;
   rc = lob.write( buf, bufLen ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to test write lob with openMode" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
}
