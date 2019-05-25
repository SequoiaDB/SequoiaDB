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
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   const CHAR *pStr                         = NULL ;
   const CHAR *pPeerCataAddr                = "192.168.20.166:11823" ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;

   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = db.getDC( dc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;


   rc = dc.activateDC() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = dc.getDetail( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "Before opration, dc detail is: " << obj.toString( FALSE, TRUE ) << endl ;

   pStr = dc.getName() ;
   cout << "DC's name is: " << pStr << endl ;

   rc = dc.createImage( pPeerCataAddr ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   obj = BSON( "Groups" << BSON_ARRAY( BSON_ARRAY( "group1" << "group1" ) << BSON_ARRAY( "group2" << "group2" ) ) ) ;
   rc = dc.attachGroups( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = dc.enableReadOnly( FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = dc.enableReadOnly( TRUE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = dc.enableImage() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = dc.activateDC() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = dc.deactivateDC() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = dc.disableImage() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = dc.enableReadOnly( FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = dc.detachGroups( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = dc.removeImage() ;
   ASSERT_EQ( SDB_OK, rc ) ;


   db.disconnect() ;
}

