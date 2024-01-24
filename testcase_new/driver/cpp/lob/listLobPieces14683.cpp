/**************************************************************
 * @Description: test case for C++ driver
 *				     seqDB-14683:listLobPieces获取lob切片
 * @Modify     : Liang xuewang Init
 *			 	     2018-03-13
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace bson ;
using namespace sdbclient ;
using namespace std ;

class listLobPiecesTest : public testBase
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
      csName = "listLobPiecesTestCs14683" ;
      clName = "listLobPiecesTestCl14683" ;
      rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = cs.createCollection( clName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = db.dropCollectionSpace( csName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
      testBase::TearDown() ;
   }
} ;

TEST_F( listLobPiecesTest, listLobPieces )
{
   INT32 rc = SDB_OK ;
   
   sdbLob lob ;
   rc = cl.createLob( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;
   const CHAR* buf = "ABCDEabcde" ;
   UINT32 len = strlen( buf ) ;
   rc = lob.write( buf, len ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   OID oid ;
   rc = lob.getOid( oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get oid" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   sdbCursor cursor ;
   rc = cl.listLobPieces( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to listLobPieces" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   ASSERT_EQ( oid, obj.getField( "Oid" ).OID() ) << "fail to check oid" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}
