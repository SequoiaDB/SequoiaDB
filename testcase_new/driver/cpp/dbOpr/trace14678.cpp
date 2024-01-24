/**************************************************************
 * @Description: test case for c++ driver
 *				     seqDB-14678:执行trace操作
 * @Modify     : Liang xuewang Init
 *			 	     2018-03-12
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class traceTest14678 : public testBase
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

TEST_F( traceTest14678, trace )
{
   INT32 rc = SDB_OK ;

   // trace start
   UINT32 traceBufSize = 256 ;
   rc = db.traceStart( traceBufSize, _sdbStaticObject ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to traceStart" ;

   // check trace status
   sdbCursor cursor ;
   rc = db.traceStatus( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to traceStatus" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   
   BOOLEAN traceStarted = obj.getField( "TraceStarted" ).Bool() ;
   ASSERT_EQ( TRUE, traceStarted ) << "fail to check TraceStarted" ;   
   SINT64 size = obj.getField( "Size" ).numberLong() ;
   ASSERT_EQ( traceBufSize*1024*1024, size ) << "fail to check Size" ;
   vector<BSONElement> components = obj.getField( "Mask" ).Array() ;
   /*const CHAR* masks[] = { 
         "auth", "bps", "cat", "cls", "dps", "mig", "msg", "net", "oss", 
         "pd", "rtn", "sql", "tools", "bar", "client", "coord", "dms", "ixm", 
         "mon", "mth", "opt", "pmd", "rest", "spt", "util", "aggr", "spd", "qgm"
      } ;
   INT32 num =  sizeof( masks ) / sizeof( masks[0] ) ;
   ASSERT_EQ( num, components.size() ) << "fail to check components num" ;
   for( INT32 i = 0;i < num;i++ )
   {
      string str = components[i].String() ;
      CHAR mask[str.size()] ;
      strcpy( mask, str.c_str()) ;
      ASSERT_STREQ( masks[i], mask ) << "fail to check component" ;
   }*/
   ASSERT_GT(components.size(), 0) << "fail to check components num" << obj.toString();
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // trace stop
   rc = db.traceStop( NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to traceStop" ;

   // check trace status
   rc = db.traceStatus( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to traceStatus" ;     
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   traceStarted = obj.getField( "TraceStarted" ).Bool() ;
   ASSERT_EQ( FALSE, traceStarted ) << "fail to check TraceStarted" ;
   rc = cursor.close() ;              
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // trace resume
   rc = db.traceResume() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to traceResume" ;

   // trace status
   rc = db.traceStatus( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to traceStatus" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   traceStarted = obj.getField( "TraceStarted" ).Bool() ;
   ASSERT_EQ( FALSE, traceStarted ) << "fail to check TraceStarted" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // trace stop in the end
   rc = db.traceStop( NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to traceStop" ;
}
