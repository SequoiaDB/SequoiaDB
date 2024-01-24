/********************************************************
 * @Description: testcase for C++ driver meta data cs
 *               seqDB-12511:创建/获取/删除/枚举CS
 *               seqDB-12659:获取不存在的CS
 * @Modify:      Liangxw
 *               2017-09-12
 ********************************************************/
#include <client.hpp>
#include <gtest/gtest.h>
#include "testcommon.hpp"
#include "arguments.hpp" 
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;

class csTest12511 : public testBase
{
protected:
   sdbCollectionSpace cs ;

   void SetUp()
   {
      testBase::SetUp() ;
   }
   void TearDown()
   {
      testBase::TearDown() ;
   }
} ;

TEST_F( csTest12511, csOpr12511 )
{
   INT32 rc = SDB_OK ;
   const CHAR* csName = "testCs12511" ;
   
   // create cs with page size
   rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;

   // get cs
   sdbCollectionSpace cs1 ;
   rc = db.getCollectionSpace( csName, cs1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs " << csName ;

   // get cs name
   ASSERT_STREQ( csName, cs.getCSName() ) << "fail to check csname" ;

   // list cs
   sdbCursor cursor ;
   rc = db.listCollectionSpaces( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to list cs" ;
   BOOLEAN found = FALSE ;
   BSONObj obj ;
   while( !cursor.next( obj ) )
   {
      string str = obj.getField( "Name" ).String() ;
      CHAR name[str.size()] ;
      strcpy( name, str.c_str() ) ;
      if( strcmp( name, csName ) == 0 )
      {
         found = TRUE ;
         break ;
      }
   }
   ASSERT_TRUE( found ) << "fail to check list cs" ;

   // drop cs
   rc = db.dropCollectionSpace( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
   rc = db.getCollectionSpace( csName, cs ) ;
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc ) << "fail to check drop cs " << csName ;
}   

TEST_F( csTest12511, createCsWithOption12511 )
{
   INT32 rc = SDB_OK ;
   const CHAR* csName = "testCs12511" ;

   // create cs with option
   BSONObj option = BSON( "PageSize" << 4096 << "LobPageSize" << 262144 ) ;
   rc = db.createCollectionSpace( csName, option, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;

   // create cl to make cs snapshot
   const CHAR* clName = "testCl12511" ;
   sdbCollection cl ;
   rc = cs.createCollection( clName, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

   // check cs option with snapshot
   sdbCursor cursor ;
   BSONObj cond = BSON( "Name" << csName ) ;
   rc = db.getSnapshot( cursor, SDB_SNAP_COLLECTIONSPACES, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to snapshot cs" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_STREQ( csName, obj.getField( "Name" ).String().c_str() )
                 << "fail to check cs name" ;   
   ASSERT_EQ( 4096, obj.getField( "PageSize" ).Int() ) 
              << "fail to check page size" ;
   ASSERT_EQ( 262144, obj.getField( "LobPageSize" ).Int() ) 
              << "fail to check lob page size" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // drop cs
   rc = db.dropCollectionSpace( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
}

TEST_F( csTest12511, notExistCs12659 )
{
   INT32 rc = SDB_OK ;
   const CHAR* csName = "notExistCs12659" ;

   // get not exist cs
   rc = db.getCollectionSpace( csName, cs ) ;
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc ) << "fail to test get not exist cs" ;
}
