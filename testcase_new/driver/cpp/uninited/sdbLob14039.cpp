/**************************************************************
 * @Description: test sdbLob without init
 *               seqDB-14039:sdbLob对象未创建时调用方法
 * @Modify     : Liang xuewang
 *               2018-01-09
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

#define BUF_LEN 128

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class sdbLob14039 : public testBase
{
protected:
   sdbLob lob ;
   void SetUp()
   {
   }
   void TearDown()
   {
   }
} ;

TEST_F( sdbLob14039, opLob )
{
   // test all function of sdbLob

   INT32 rc = SDB_OK ;
   rc = lob.close() ;
   EXPECT_EQ( SDB_OK, rc ) << "close lob should be ok" ;
   CHAR readBuf[ BUF_LEN ] ;
   CHAR writeBuf[ BUF_LEN ] ;
   UINT32 readLen ;
   rc = lob.read( BUF_LEN, readBuf, &readLen ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "read lob shouldn't succeed" ;
   rc = lob.write( writeBuf, BUF_LEN ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "write lob shouldn't succeed" ;
   rc = lob.seek( 0, SDB_LOB_SEEK_SET ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "seek lob shouldn't succeed" ;
   rc = lob.lock( 0, -1 ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "lock lob shouldn't succeed" ;
   rc = lob.lockAndSeek( 0, -1 ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "lockAndSeek lob shouldn't succeed" ;
   EXPECT_TRUE( lob.isClosed() ) << "isClosed should be TRUE" ;
   OID oid ;
   EXPECT_TRUE( lob.getOid() == oid ) << "getOid should be OID()" ;
   EXPECT_EQ( -1, lob.getSize() ) << "getSize should be -1" ;
   EXPECT_EQ( -1, lob.getCreateTime() ) << "getCreateTime should be -1" ;
   EXPECT_EQ( -1, lob.getModificationTime() ) << "getModificationTime should be -1" ;
   EXPECT_EQ( -1, lob.getPiecesInfoNum() ) << "getPiecesInfoNum should be -1" ;
   BSONArray arr ;
   EXPECT_TRUE( arr == lob.getPiecesInfo() ) << "getPiecesInfo should be BSONArray()" ;
}
