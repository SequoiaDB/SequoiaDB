/*****************************************************************
 * @Description:    test case for C++ driver meta data cl
 *                  seqDB-12660:创建主表、挂载/去挂载子表
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

class clTest12660 : public testBase
{
protected:
   sdbCollectionSpace cs ;
   const CHAR* csName ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "testCs12660" ;
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

TEST_F( clTest12660, mainCl12660 )
{
   INT32 rc = SDB_OK ;

   const CHAR* mainClName = "testMainCl12660" ;
   const CHAR* subClName = "testSubCl12660" ;
   sdbCollection mainCl ;
   sdbCollection subCl ;

   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone." << endl ;
      return ;
   }

   // create main cl
   BSONObj option = BSON( "ShardingKey" << BSON( "a" << 1 ) << "ShardingType" << "range" << 
                          "IsMainCL" << true << "Partition" << 1024 ) ;
   rc = cs.createCollection( mainClName, option, mainCl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create main cl " << mainClName ;

   // check create cl option 
   sdbCursor cursor ;
   CHAR mainClFullName[100] ;
   sprintf( mainClFullName, "%s%s%s", csName, ".", mainClName ) ;
   BSONObj cond = BSON( "Name" << mainClFullName ) ;
   rc = db.getSnapshot( cursor, SDB_SNAP_CATALOG, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get snapshot cata" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_STREQ( "range", obj.getField( "ShardingType" ).String().c_str() ) 
                 << "fail to check create cl option" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // create sub cl
   option = BSON( "ShardingKey" << BSON( "a" << 1 ) << "ShardingType" << "hash" ) ;
   rc = cs.createCollection( subClName, option, subCl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create sub cl " << subClName ;

   // attach sub cl
   option = BSON( "LowBound" << BSON( "a" << 0 ) << "UpBound" << BSON( "a" << 100 ) ) ;
   CHAR subClFullName[100] ;
   sprintf( subClFullName, "%s%s%s", csName, ".", subClName ) ;
   rc = mainCl.attachCollection( subClFullName, option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to attatch sub cl " << subClFullName ;
   
   // check attach with sub cl snapshot
   cond = BSON( "Name" << subClFullName ) ;
   rc = db.getSnapshot( cursor, SDB_SNAP_CATALOG, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get snapshot cata" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_STREQ( mainClFullName, obj.getField( "MainCLName" ).String().c_str() )
                 << "fail to check attatch with sub cl snapshot" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;  
   
   // check attatch with main cl snapshot
   cond = BSON( "Name" << mainClFullName ) ;
   rc = db.getSnapshot( cursor, SDB_SNAP_CATALOG, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get snapshot cata" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   vector<BSONElement> cataInfo = obj.getField( "CataInfo" ).Array() ;
   ASSERT_EQ( 1, cataInfo.size() ) << "fail to check ";
   ASSERT_EQ( 100, cataInfo[0].Obj().getField( "UpBound" ).Obj().getField( "a" ).Int() )
              << "fail to check attatch with main cl snapshot" ;
   cataInfo.clear() ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   
   // detach sub cl
   rc = mainCl.detachCollection( subClFullName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to detach sub cl " << subClFullName ;
   
   // check detach with sub cl snapshot
   cond = BSON( "Name" << subClFullName ) ;
   rc = db.getSnapshot( cursor, SDB_SNAP_CATALOG, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get snapshot cata" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_TRUE( obj.getField( "MainCLName" ).eoo() ) << "fail to check detach with sub cl snapshot" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   
   // check detach with main cl snapshot
   cond = BSON( "Name" << mainClFullName ) ;
   rc = db.getSnapshot( cursor, SDB_SNAP_CATALOG, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get snapshot cata" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   cataInfo = obj.getField( "CataInfo" ).Array() ;
   ASSERT_EQ( 0, cataInfo.size() ) << "fail to check detach with main cl snapshot" ;

   // drop sub cl, main cl
   rc = cs.dropCollection( subClName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop sub cl " << subClName ;
   rc = cs.dropCollection( mainClName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop main cl " << mainClName ;
}
