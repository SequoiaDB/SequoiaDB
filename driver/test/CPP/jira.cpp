#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;

TEST( jira, bug_8345 )
{
   sdb db ;
   BSONObj result ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;

   // connect to database
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_TRUE( rc==SDB_OK ) ;
   db.dropCollectionSpace( COLLECTION_SPACE_NAME ) ;
   // case 1: test get last error
   {
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCollection cl2 ;
   rc = db.createCollectionSpace( COLLECTION_SPACE_NAME, SDB_PAGESIZE_4K, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
   rc = cs.createCollection( "test", cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
   rc = cs.createCollection( "test", cl2 ) ;
   rc = db.getLastErrorObj( result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "failed to get last error object" ;
   printf("error obj is: %s\n", result.toString(false, true, false).c_str() ) ;
   }
   rc = db.getLastErrorObj( result ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "failed to get last error object" ;

   // case 2: test get last result
   {
   sdbCollection cl ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.getCollection( COLLECTION_FULL_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.insert( BSON( "a" << 1 ) ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.getLastResultObj( result ) ;
   ASSERT_EQ( SDB_OK, rc ) << "failed to get last error object" ;
   printf("result obj is: %s\n", result.toString(false, true, false).c_str() ) ; 
   }
   rc = db.getLastResultObj( result ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "failed to get last result object" ;

   rc = db.dropCollectionSpace( COLLECTION_SPACE_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
   db.disconnect() ;
}
