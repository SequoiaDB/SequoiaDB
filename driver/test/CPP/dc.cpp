#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;


TEST( dc, not_connect )
{
   sdbDataCenter dc ;
   BSONObj obj ;
   INT32 rc = SDB_OK ;

   rc = dc.getDetail( obj ) ;
   ASSERT_EQ( SDB_NOT_CONNECTED, rc ) ;
}

TEST( dc, all_api_test )
{
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbDataCenter dc ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   const CHAR *pStr                         = NULL ;
   const CHAR *pPeerCataAddr                = "192.168.20.166:11823" ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;

   // connect to database
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // getDC
   rc = db.getDC( dc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;


   rc = dc.activateDC() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // getDetail
   rc = dc.getDetail( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "Before opration, dc detail is: " << obj.toString( FALSE, TRUE ) << endl ;

   // getName
   pStr = dc.getName() ;
   cout << "DC's name is: " << pStr << endl ;

   // createImage
   rc = dc.createImage( pPeerCataAddr ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // attachGroups
   obj = BSON( "Groups" << BSON_ARRAY( BSON_ARRAY( "group1" << "group1" ) << BSON_ARRAY( "group2" << "group2" ) ) ) ;
   rc = dc.attachGroups( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // disableReadonly
   rc = dc.enableReadOnly( FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // enableReadonly 
   rc = dc.enableReadOnly( TRUE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // enableImage
   rc = dc.enableImage() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // activateDC
   rc = dc.activateDC() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // deactivateDC
   rc = dc.deactivateDC() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // disableImage
   rc = dc.disableImage() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // disableReadonly
   rc = dc.enableReadOnly( FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // detachGroups
   rc = dc.detachGroups( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // removeImage
   rc = dc.removeImage() ;
   ASSERT_EQ( SDB_OK, rc ) ;


   // disconnect the connection
   db.disconnect() ;
}

