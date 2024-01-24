/*****************************************************************************
 * @Description : testcase for numberLong
 *                seqDB-7823:c++_strict格式的参数校验
 *                seqDB-7822:c++_输入strict格式，查询显示
 *                seqDB-7824:c++_strict格式的边界值校验
 * @Modify List : Ting YU
 *                2016-03-29
 *****************************************************************************/
#include <stdio.h>
#include <gtest/gtest.h>
#include <string>
#include <iostream>
#include <sstream>
#include <client.hpp>
#include "testcommon.hpp"
#include "arguments.hpp"

using namespace std ;
using namespace sdbclient ;
using namespace bson ; 

TEST( numberLongTest7822, boundary7824 )
{         
   //create cl
   INT32 rc = SDB_OK ;
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   const CHAR* csName = "numberLongTestCs7822" ;
   const CHAR* clName = "numberLongTestCl7822" ;
   rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect" ;
   rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;   

   SINT64 longVals[] = { 9223372036854775807, -9223372036854775808 } ;
   INT32 size = sizeof(longVals) / sizeof(longVals[0]) ;
   for( INT32 i = 0;i < size;i++ )
   {
      SINT64 longVal = longVals[i] ;
      stringstream ss ;
      ss << longVal ;
      string longValStr = ss.str() ;

      //json to bson
      string recJson = "{ \"a\": { \"$numberLong\": \"" + longValStr + "\" } }" ;   
      BSONObj recBson ;
      fromjson( recJson, recBson, true ) ;
      ASSERT_EQ( longVal, recBson.getField( "a" ).Long() ) << "fail to check value" ;

      rc = cl.del() ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to del" ;   
      rc = cl.insert( recBson ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

      INT32 cnt = 0 ;
      sdbCursor cursor ;
      rc = cl.query( cursor ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
      BSONObj qryRec ;
      while( !cursor.next( qryRec ) )
      {  
         SINT64 qryVal = qryRec.getField( "a" ).Long() ;
         ASSERT_EQ( longVal, qryVal ) << "fail to check query result" ;     
         i++ ;
      }
      ASSERT_EQ( 1, i ) << "fail to check query num" ; 

      rc = cursor.close() ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   }

   rc = db.dropCollectionSpace( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
   db.disconnect() ; 
}

TEST( numberLongTest7822, outofBoundary7824 )
{
   INT32 rc = SDB_OK ;
   string recJson;
   BSONObj recBson; 

   //json to bson
   recJson = "{ \"a\": { \"$numberLong\": \"9223372036854775808\" } }" ;   
   rc = fromjson( recJson, recBson, true ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to check numberLong max+1" ;

   //json to bson
   recJson = "{ \"a\": { \"$numberLong\": \"-9223372036854775809\" } }" ;
   rc = fromjson( recJson, recBson, true ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to check numberLong min-1" ;  

}

TEST( numberLongTest7822, errorFormat7823 )
{             
   INT32 rc = SDB_OK ;
   string recJson ;
   BSONObj recBson ;

   //1. error fomart: {"$numberLong":"123.5"}       
   recJson = "{ \"a\": { \"$numberLong\": \"123.5\" } }" ;  
   rc = fromjson( recJson, recBson, true ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to check errorFormat point" ;    

   //2. error fomart: {"$numberLong":123}       
   recJson = "{ \"a\": { \"$numberLong\": 123.5 } }" ;
   rc = fromjson( recJson, recBson, true ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "fail to check errorFormat number" ;       
}
