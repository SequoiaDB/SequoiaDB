/*****************************************************************
 * @Description:    test case for C++ driver meta data cl
 *                  seqDB-12512:创建/获取/枚举/修改/删除普通CL
 *                  seqDB-12747:分别使用db/cs获取存在/不存在的cl
 * @Modify:         Liang xuewang Init
 *                  2017-09-12
 *****************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class clTest12512 : public testBase
{
protected:
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   const CHAR* csName ;
   const CHAR* clName ;
   const CHAR* clFullName ;

   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "testCs12512" ;
      clName = "testCl12512" ;
      clFullName = "testCs12512.testCl12512" ;
      rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
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

TEST_F( clTest12512, clOpr12512 )
{
   INT32 rc = SDB_OK ;
   
   // create cl
   rc = cs.createCollection( clName, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

   // use db to get cl with clfullname
   sdbCollection cl1 ;
   rc = db.getCollection( clFullName, cl1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl " << clFullName ;

   // get name
   ASSERT_STREQ( clFullName, cl.getFullName() ) << "fail to check cl fullname" ;
   ASSERT_STREQ( csName, cl.getCSName() ) << "fail to check cs name" ;
   ASSERT_STREQ( clName, cl.getCollectionName() ) << "fail to check cl name" ;

   // list cl
   sdbCursor cursor ;
   rc = db.listCollections( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to list cl" ;
   BOOLEAN found = FALSE ;
   BSONObj obj ;
   while( !cursor.next(obj) )
   {  
      string str = obj.getField( "Name" ).String() ;
      CHAR tmp[ str.size() ] ;
      strcpy( tmp, str.c_str() ) ;
      if( strcmp( tmp, clFullName ) == 0 )
      {
         found = TRUE ;
         break ;
      }
   }
   ASSERT_TRUE( found ) << "fail to check list cl" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // alter cl
   if( !isStandalone( db ) )
   {
      BSONObj option = BSON( "ShardingKey" << BSON( "a" << 1 ) ) ;
      rc = cl.alterCollection( option ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to alter cl " << clFullName ;
      BSONObj cond = BSON( "Name" << clFullName ) ;
      sdbCursor cursor1 ;
      rc = db.getSnapshot( cursor1, SDB_SNAP_CATALOG, cond ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get snapshot cata" ;
      rc = cursor1.next( obj ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
      ASSERT_EQ( 1, obj.getField( "ShardingKey" ).Obj().getField( "a" ).Int() ) << "fail to check alter cl" ;
      rc = cursor1.close() ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   }

   // insert record
   BSONObj doc = BSON( "a" << 50 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert doc into cl " << clFullName ;

   // use cs to get cl
   sdbCollectionSpace cs1 ;
   rc = db.getCollectionSpace( csName, cs1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs " << csName ;
   sdbCollection cl2 ;
   rc = cs1.getCollection( clName, cl2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl " << clName ;

   // drop cl
   rc = cs1.dropCollection( clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
}

TEST_F( clTest12512, notExist12747 )
{
   INT32 rc = SDB_OK ;
   const CHAR* notExistClName = "notExistCl12747" ;
   CHAR notExistClFullName[100] ;
   sprintf( notExistClFullName, "%s%s%s", csName, ".", notExistClName ) ;   

   rc = cs.getCollection( notExistClName, cl ) ;
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) << "fail to test get not exist cl " << clName ;
   rc = db.getCollection( notExistClFullName, cl ) ;
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) << "fail to test get not exist cl " << clName ;
}
