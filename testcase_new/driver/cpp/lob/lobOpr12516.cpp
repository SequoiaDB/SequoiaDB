/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12516:打开/写/读/关闭lob/获取lob信息
 *                 seqDB-12547:指定oid创建lob
 *                 seqDB-12748:写lob大小超过驱动最大写lob消息长度（2M）
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

class lobOprTest12516 : public testBase
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
      csName = "lobOprTestCs12516" ;
      clName = "lobOprTestCl12516" ;
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

TEST_F( lobOprTest12516, lobOpr12516 )
{
   INT32 rc = SDB_OK ;

   time_t tm = time( NULL ) ;

   // create lob
   sdbLob lob ;
   rc = cl.createLob( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;

   // get oid
   OID oid = lob.getOid() ;

   // write lob
   const CHAR* buf = "0123456789ABCDEFabcdef" ;
   INT32 lobSize = strlen( buf ) ;
   rc = lob.write( buf, lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;

   // close lob
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // open lob and get time size
   rc = cl.openLob( lob, oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;
   UINT64 createTime = lob.getCreateTime() ;
   ASSERT_GT( createTime, tm ) << "fail to check time" ; 
   SINT64 size = lob.getSize() ;
   ASSERT_EQ( lobSize, size ) << "fail to check size" ;

   // read lob
   CHAR readBuf[100] = { 0 } ;
   UINT32 len = 10 ;
   UINT32 readLen ;
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( len, readLen ) << "fail to check lob read len" ;
   ASSERT_STREQ( "0123456789", readBuf ) << "fail to check read buf" ;

   // close lob
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   // list lobs
   sdbCursor cursor ;
   rc = cl.listLobs( cursor ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to list lobs" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( oid, obj.getField( "Oid" ).OID() ) << "fail to check oid" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // remove lob
   rc = cl.removeLob( oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to remove lob" ;
   rc = cl.openLob( lob, oid ) ;
   ASSERT_EQ( SDB_FNE, rc ) << "fail to check remove lob" ;
}

TEST_F( lobOprTest12516, createLobWithOid12547 )
{
   INT32 rc = SDB_OK ;

   sdbLob lob ;
   OID oid( "0123456789ABCDEFabcdef01" ) ;
   cout << "oid: " << oid << endl ;
   rc = cl.createLob( lob, &oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;
   INT32 lobSize = 200 * 1024 ;
   string str( lobSize, 'x' ) ;
   const CHAR* buf = str.c_str() ;
   rc = lob.write( buf, lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
   ASSERT_EQ( oid, lob.getOid() ) << "fail to get lob oid" ;
}

TEST_F( lobOprTest12516, writeLobLarge12748 )
{
   INT32 rc = SDB_OK ;

   sdbLob lob ;
   rc = cl.createLob( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;
   ASSERT_EQ( 0, lob.getSize() ) << "fail to check lob size before write" ;

   INT32 lobSize = 3 * 1024 * 1024 ;
   string str( lobSize, 'x' ) ;
   const CHAR* buf = str.c_str() ;
   rc = lob.write( buf, lobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   
   ASSERT_EQ( lobSize, lob.getSize() ) << "fail to check lob size after write" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
   ASSERT_EQ( lobSize, lob.getSize() ) << "fail to check lob size after close" ;
}
