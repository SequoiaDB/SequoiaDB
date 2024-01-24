/**************************************************************************
 * @Description:   test case for C++ driver
 *                 seqDB-12676:exec执行查询SQL
 *                 seqDB-12678:execUpdate执行更新SQL
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

class sqlTest12676 : public testBase
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
      csName = "sqlTestCs12676" ;
      clName = "sqlTestCl12676" ;
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

TEST_F( sqlTest12676, exec12676 )
{
   INT32 rc = SDB_OK ;

   BSONObj doc = BSON( "a" << 1 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   CHAR sql[100] ;
   sprintf( sql, "%s%s%s%s%s", "select * from ", csName, ".", clName, " where a = 1" ) ;
   cout << "sql: " << sql << endl ;
   sdbCursor cursor ;
   rc = db.exec( sql, cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to exec sql" ;

   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 1, obj.getField( "a" ).Int() ) << "fail to check a" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}

TEST_F( sqlTest12676, execUpdate12678 )
{
   INT32 rc = SDB_OK ;

   CHAR sql[100] ;
   sprintf( sql, "%s%s%s%s%s", "insert into ", csName, ".", clName, "(a) values(10)" ) ;
   cout << "sql: " << sql << endl ;

   rc = db.execUpdate( sql ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to execUpdate" ;

   sdbCursor cursor ;
   rc = cl.query( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_EQ( 10, obj.getField( "a" ).Int() ) << "fail to check a" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}
