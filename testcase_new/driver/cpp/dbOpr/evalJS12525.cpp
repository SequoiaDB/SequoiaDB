/**************************************************************
 * @Description: test backup
 *               seqDB-12525:执行返回结果为各种类型的JS语句
 * @Modify     : Suqiang Ling
 *               2017-09-11
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include <vector>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;
using namespace sdbclient ;
using namespace std ;
using namespace bson ;

class evalJSTest12525 : public testBase
{
protected:
   void SetUp()
   {
      testBase::SetUp() ;
   }
   void TearDown()
   {
      testBase::TearDown() ;
   }
} ;

TEST_F( evalJSTest12525, void12525 )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ; 
   }
   INT32 rc = SDB_OK ;
   
   const CHAR* code = "db.flushConfigure({Global:false})" ;
   SDB_SPD_RES_TYPE type ;
   sdbCursor cursor ;
   BSONObj errmsg ;
   rc = db.evalJS( code, type, cursor, errmsg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to eval js" ;
   ASSERT_EQ( SDB_SPD_RES_TYPE_VOID, type ) ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to test get next" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}

TEST_F( evalJSTest12525, str12525 )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ; 
   }
   INT32 rc = SDB_OK ;
  
   const CHAR* code = "a = \"abc\"" ;
   SDB_SPD_RES_TYPE type ;
   sdbCursor cursor ;
   BSONObj errmsg ;
   rc = db.evalJS( code, type, cursor, errmsg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to eval js" ;
   ASSERT_EQ( SDB_SPD_RES_TYPE_STR, type ) ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( "abc", obj.getField( "value" ).String() ) << "fail to check result" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ; 
}

TEST_F( evalJSTest12525, number12525 )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ; 
   }
   INT32 rc = SDB_OK ;

   const CHAR* code = "a = 123" ;
   SDB_SPD_RES_TYPE type ;
   sdbCursor cursor ;
   BSONObj errmsg ; 
   rc = db.evalJS( code, type, cursor, errmsg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to eval js" ;
   ASSERT_EQ( SDB_SPD_RES_TYPE_NUMBER, type ) ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 123, obj.getField( "value" ).Int() ) << "fail to check result" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}

TEST_F( evalJSTest12525, obj12525 )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ; 
   }
   INT32 rc = SDB_OK ;

   const CHAR* code = "a = { a: 1 }" ;
   SDB_SPD_RES_TYPE type ;
   sdbCursor cursor ;
   BSONObj errmsg ; 
   rc = db.evalJS( code, type, cursor, errmsg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to eval js" ;
   ASSERT_EQ( SDB_SPD_RES_TYPE_OBJ, type ) ; 
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 1, obj.getField( "value" ).Obj().getField( "a" ).Int() ) << "fail to check result" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}

TEST_F( evalJSTest12525, bool12525 )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ; 
   }
   INT32 rc = SDB_OK ;
   const CHAR* code = "a = true" ;
   SDB_SPD_RES_TYPE type ;
   sdbCursor cursor ;
   BSONObj errmsg ; 
   rc = db.evalJS( code, type, cursor, errmsg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to eval js" ;               
   ASSERT_EQ( SDB_SPD_RES_TYPE_BOOL, type ) ; 
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;              
   ASSERT_TRUE( obj.getField( "value" ).Bool() ) << "fail to check result" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}

TEST_F( evalJSTest12525, recordSet12525 )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ; 
   }
   INT32 rc = SDB_OK ;

   sdbCollectionSpace cs ;
   sdbCollection cl ;
   const CHAR* csName = "evalJSTestCs12525" ;
   const CHAR* clName = "evalJSTestCl12525" ;
   rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj doc = BSON( "a" << 10 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   
   const CHAR* code = "db.evalJSTestCs12525.evalJSTestCl12525.find()" ;
   SDB_SPD_RES_TYPE type ;
   sdbCursor cursor ;
   BSONObj errmsg ; 
   rc = db.evalJS( code, type, cursor, errmsg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to eval js" ;
   ASSERT_EQ( SDB_SPD_RES_TYPE_RECORDSET, type ) ; 
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 10, obj.getField( "a" ).Int() ) << "fail to check result" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
      
   rc = db.dropCollectionSpace( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
}

TEST_F( evalJSTest12525, cs12525 )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ; 
   }
   INT32 rc = SDB_OK ;

   sdbCollectionSpace cs ;
   const CHAR* csName = "evalJSTestCs12525" ;
   rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;

   const CHAR* code = "db.evalJSTestCs12525" ;
   SDB_SPD_RES_TYPE type ;
   sdbCursor cursor ;
   BSONObj errmsg ; 
   rc = db.evalJS( code, type, cursor, errmsg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to eval js" ;
   ASSERT_EQ( SDB_SPD_RES_TYPE_SPECIALOBJ, type ) ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( "SdbCS", obj.getField( "className" ).String() ) 
         << "wrong class name" ;
   ASSERT_EQ( csName, obj.getField( "value" ).Obj().getField( "_name" ).String() ) 
         << "fail to check result" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
             
   rc = db.dropCollectionSpace( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ; 
}

TEST_F( evalJSTest12525, cl12525 )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ; 
   }
   INT32 rc = SDB_OK ;
   
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   const CHAR* csName = "evalJSTestCs12525" ;
   const CHAR* clName = "evalJSTestCl12525" ;
   rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   const CHAR* code = "db.evalJSTestCs12525.evalJSTestCl12525" ;
   SDB_SPD_RES_TYPE type ;
   sdbCursor cursor ;
   BSONObj errmsg ; 
   rc = db.evalJS( code, type, cursor, errmsg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to eval js" ;
   ASSERT_EQ( SDB_SPD_RES_TYPE_SPECIALOBJ, type ) ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( "SdbCollection", obj.getField( "className" ).String() ) << "wrong className" ;
   ASSERT_EQ( "evalJSTestCs12525.evalJSTestCl12525", 
              obj.getField( "value" ).Obj().getField( "_name" ).String() ) << "fail to check result" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
             
   rc = db.dropCollectionSpace( csName ) ;   
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ; 
}

TEST_F( evalJSTest12525, rg12525 )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   const CHAR* code = "db.getCoordRG()" ;
   SDB_SPD_RES_TYPE type ;
   sdbCursor cursor ;
   BSONObj errmsg ; 
   rc = db.evalJS( code, type, cursor, errmsg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to eval js" ;
   ASSERT_EQ( SDB_SPD_RES_TYPE_SPECIALOBJ, type ) ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( "SdbReplicaGroup", obj.getField( "className" ).String() ) << "wrong className" ;
   ASSERT_EQ( "SYSCoord", obj.getField( "value" ).Obj().getField( "_name" ).String() ) 
         << "fail to check result" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}

TEST_F( evalJSTest12525, rgNode12525 )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }
   
   vector<string> groups ;
   rc = getGroups( db, groups ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   const CHAR* rgName = groups[0].c_str() ;
   sdbReplicaGroup rg ;
   sdbNode node ;
   rc = db.getReplicaGroup( rgName, rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get rg " << rgName ;
   rc = rg.getMaster( node ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get master node" ;
   CHAR nodeFullName[128] ;
   sprintf( nodeFullName, "%s%s%s", rgName, ":", node.getNodeName() ) ;

   CHAR code[100] ;
   sprintf( code, "%s%s%s", "db.getRG( \"", rgName, "\" ).getMaster()" ) ;
   SDB_SPD_RES_TYPE type ;
   sdbCursor cursor ;
   BSONObj errmsg ; 
   rc = db.evalJS( code, type, cursor, errmsg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to eval js" ;
   ASSERT_EQ( SDB_SPD_RES_TYPE_SPECIALOBJ, type ) ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( "SdbNode", obj.getField( "className" ).String() ) << "wrong className" ;
   ASSERT_EQ( nodeFullName, obj.getField( "value" ).Obj().getField( "_nodename" ).String() ) 
         << "fail to check result" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}
