/**************************************************************************
 * @Description:   seqDB-13416: 获取lob修改时间
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

class getLobModTime13416 : public testBase
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
      pCsName = "getLobModTime13416" ;
      pClName = "getLobModTime13416" ;

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

TEST_F( getLobModTime13416, lobWrite )
{
   INT32 rc = SDB_OK ;
   sdbLob lob ;
   OID oid ;
   UINT64 createTime ;
   UINT64 initModTime ;
   UINT64 writeModTime ;
   UINT64 readModTime ;
   const CHAR *writeBuf = "0123456789ABCDEabcde" ;
   const INT32 wBufSize = strlen( writeBuf ) ;
   const UINT32 rBufSize = wBufSize ;
   CHAR readBuf[ rBufSize ] ;
   UINT32 read ;

   // create lob
   rc = cl.createLob( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = lob.close() ;
   createTime = lob.getCreateTime() ;
   initModTime = lob.getModificationTime() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   EXPECT_LE( createTime, initModTime ) 
         << "wrong modification time after init" ;

   // modify lob
   usleep(1000);
   rc = lob.getOid( oid ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.openLob( lob, oid, SDB_LOB_WRITE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = lob.write( writeBuf, wBufSize ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   writeModTime = lob.getModificationTime() ;
   EXPECT_LT( initModTime, writeModTime ) 
         << "wrong modification time after modifying" ;

   // not modify lob
   rc = cl.openLob( lob, oid, SDB_LOB_READ ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = lob.read( rBufSize, readBuf, &read ) ;
   ASSERT_EQ( rBufSize, read ) << "long read length is wrong" ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   readModTime = lob.getModificationTime() ;
   EXPECT_EQ( writeModTime, readModTime ) 
         << "wrong modification time when no modification" ;
}
