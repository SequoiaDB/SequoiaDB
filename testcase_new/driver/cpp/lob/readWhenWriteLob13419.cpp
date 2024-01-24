/**************************************************************************
 * @Description:   seqDB-13419: 加锁写入过程中读取lob
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

class readWhenWriteLob13419 : public testBase
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
      pCsName = "readWhenWriteLob13419" ;
      pClName = "readWhenWriteLob13419" ;

      // create cs, cl
      BSONObj opts = BSON( "PreferedInstance" << "M" ) ;
      rc = db.setSessionAttr( opts ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to set PreferedInstance" ;
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

TEST_F( readWhenWriteLob13419, readLob )
{
   INT32 rc             = SDB_OK ;
   sdbLob wLob ;
   sdbLob rLob ;
   OID oid ;
   const CHAR *writeBuf = "0123456789ABCDEabcde" ;
   const INT32 bufSize  = strlen( writeBuf ) ;
   CHAR readBuf[ bufSize ] ;
   UINT32 read ;

   // create a lob
   rc = cl.createLob( wLob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;
   rc = wLob.getOid( oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get oid" ;
   rc = wLob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // lock and write lob
   rc = cl.openLob( wLob, oid, SDB_LOB_WRITE ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;
   rc = wLob.lockAndSeek( 0, bufSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to lock and seek lob" ;
   rc = wLob.write( writeBuf, bufSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   ASSERT_EQ( bufSize, wLob.getSize() ) << "wrong lob size" ;

   // read lob when writing does not finish
   rc = cl.openLob( rLob, oid, SDB_LOB_READ ) ;
   ASSERT_EQ( SDB_LOB_IS_IN_USE, rc ) << "error code must be SDB_LOB_IS_IN_USE" ;
   rc = rLob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
   rc = wLob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // check lob write ok
   rc = cl.openLob( rLob, oid, SDB_LOB_READ ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;
   rc = rLob.read( bufSize, readBuf, &read ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( bufSize, read ) << "long read length is wrong" ;
   rc = rLob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
   ASSERT_EQ( 0, memcmp( writeBuf, readBuf, bufSize ) ) << "wrong lob content" ;
}
