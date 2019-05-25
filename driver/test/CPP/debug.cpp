#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;

#define USERNAME               "SequoiaDB"
#define PASSWORD               "SequoiaDB"

#define NUM_RECORD             5

TEST ( debug, test )
{
   ASSERT_TRUE( 1 == 1 ) ;
}


void _genObjectForInOperator( BSONObj &obj )
{
   BSONObj obj2 ;
   BSONObj sub ;
   BSONObj sub2 ;
   BSONObjBuilder bob ;
   BSONObjBuilder bob2 ;
   BSONObjBuilder bob3 ;
   BSONArrayBuilder bab ;
   BSONArray arr ;

   sub = bob.appendBinData( "a", 4, BinDataGeneral, "aaa" ).obj() ;
   arr = bab.append( sub ).append( sub ).arr() ;
   sub2 = bob2.appendArray( "$in", arr ).obj() ;
   obj = bob3.append( "age", sub2 ).obj() ;

   cout << "obj is: " << obj.toString(false,true).c_str() << endl ;  
} 

void _genObject( BSONObj &obj ) 
{
   BSONObjBuilder bob ;
   BSONObjBuilder bob2 ;
   BSONArrayBuilder bab ;

   BSONObj tmp = BSON( "$binary" << "aaa" << "$type" << 0 ) ;
   BSONObj tmp2 = BSON( "$binary" << "bbb" << "$type" << 0 ) ;
   bab.append( tmp ).append( tmp2 ) ;
   obj = bob2.append( "a", bob.append( "$in", bab.arr() ).obj()).obj() ;
   cout << "obj is: " << obj.toString(false,true).c_str() << endl ;  
}


TEST( debug, SdbIsValid )
{
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   BSONObj obj ;
   const CHAR *pCSName = "debug" ;
   const CHAR *pCLName = "debug" ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BOOLEAN result = FALSE ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   db.dropCollectionSpace( COLLECTION_SPACE_NAME ) ; 
   rc = db.createCollectionSpace( COLLECTION_SPACE_NAME, SDB_PAGESIZE_4K, cs ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ; 

   rc = cs.createCollection( COLLECTION_NAME, cl ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ; 
   rc = cs.getCollection( COLLECTION_NAME, cl ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl" ; 

   _genObject( obj ) ;
   
   rc = cl.query( cursor, obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "failed to query" ;    
   

   rc = db.dropCollectionSpace( COLLECTION_SPACE_NAME ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ; 
   db.disconnect() ;
}
