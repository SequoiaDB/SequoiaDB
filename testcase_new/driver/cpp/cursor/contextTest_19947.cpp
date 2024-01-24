/**************************************************************************
 * @Description:   test case for C++ driver
 *                  seqDB-19947 : query/queryone结束时，检查对应的context是否存在
 * @Modify:        wenjing wang Init
 *                 2019-10-10
 **************************************************************************/
#include <iostream>
#include <gtest/gtest.h>
#include <client.hpp>
#include "testcommon.hpp"
#include "testBase.hpp"
#include <vector>

using namespace sdbclient ;

int loadData(sdbCollection&  cl)
{
   std::vector<bson::BSONObj> objs ;
   for ( int i = 0; i < 10; ++i )
   {
      objs.push_back( BSON("id" << i ));
   }	  
   
   INT32 rc = cl.bulkInsert( 0, objs) ;

   return rc ;   
}

class contextTest_19947 : public testBase 
{
protected:
   sdbCollection cl ;
   const char *clName ;
   
   void SetUp()
   {
      INT32 rc = SDB_OK ;
      testBase::SetUp() ;
	  
	  clName = "contextTest_19947" ;
	  
	  rc = cs.createCollection( clName, cl ) ;
	  ASSERT_EQ( SDB_OK, rc ) << "fail to create/getCollection " << clName ;
	  
	  rc = loadData( cl ) ;
	  ASSERT_EQ( SDB_OK, rc ) << "fail to loadData " << clName ;
   }
   
   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = cs.dropCollection( clName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl " << clName ;
      } 
      testBase::TearDown() ;
   }
} ;


int getContextNum( sdb& db )
{
   int contextNum = 0 ;
   sdbCursor cursor ;
   bson::BSONObj cond = BSON("Contexts.Type" << "DATA" ) ;
   
   INT32 rc = db.getSnapshot( cursor, 0, cond ) ;
   if ( rc != SDB_OK )
   {
      std::cout << "getSnapshot return = " << rc << std::endl ; 
      return -1 ;
   }
   
   bson::BSONObj obj ;
   while ( cursor.next(obj) == 0 )
   {
      contextNum++ ;
   }
   cursor.close() ; 
   return contextNum ;
}


TEST_F( contextTest_19947, query )
{
   sdbCursor cursor ;
   INT32 rc = cl.query( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query " << clName ;
   
   bson::BSONObj obj ;
   cursor.next( obj ) ;
   cursor.close() ;
   
   ASSERT_EQ( getContextNum(db), 0 ) ;
   
   rc = cl.query( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query " << clName ;
   while ( cursor.next( obj ) == 0 ) ;
   ASSERT_EQ( getContextNum(db), 0 ) ;
}

TEST_F( contextTest_19947, queryOne )
{
   bson::BSONObj obj ;
   INT32 rc = cl.queryOne(obj) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to queryOne " << clName ;
   
   ASSERT_EQ( getContextNum(db), 0 ) ;
}

TEST_F( contextTest_19947, queryWithFlagExplain )
{
   sdbCursor cursor ;
   INT32 rc = cl.query( cursor, _sdbStaticObject, _sdbStaticObject, _sdbStaticObject, _sdbStaticObject, 0, -1, 0x00000400 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query " << clName ;
   
   bson::BSONObj obj ;
   cursor.next( obj ) ;
   cursor.close() ;
   
   ASSERT_EQ( getContextNum(db), 0 ) ;
   
   rc = cl.query( cursor, _sdbStaticObject, _sdbStaticObject, _sdbStaticObject, _sdbStaticObject, 0, -1, 0x00000400 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query " << clName ;
   while ( cursor.next( obj ) == 0 ) ;
   ASSERT_EQ( getContextNum(db), 0 ) ;
   
   rc = cl.queryOne(obj, _sdbStaticObject, _sdbStaticObject, _sdbStaticObject, _sdbStaticObject, 0, 0x00000400 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to queryOne " << clName ;
   
   ASSERT_EQ( getContextNum(db), 0 ) ;
}


