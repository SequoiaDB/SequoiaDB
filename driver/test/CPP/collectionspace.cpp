#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;


#define NUM_RECORD             5

TEST(collectionspace, not_connect)
{
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   INT32 rc = SDB_OK ;
   rc = cs.getCollection( COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_NOT_CONNECTED, rc ) ;
}

TEST(collectionspace,getCollection)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cs.getCollection( COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   clName = cl.getCollectionName() ;
   cout<<"The cl we got is : "<<clName<<endl ;
   connection.disconnect() ;
}

TEST(collectionspace,createCollection_without_Sharding_and_replSize)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cs.dropCollection( COLLECTION_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cs.createCollection( COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   clName = cl.getCollectionName() ;
   cout<<"The cl we created is : "<<clName<<endl ;
   connection.disconnect() ;
}

TEST(collectionspace,dropCollection)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCollection cl1 ;

   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cs.getCollection( COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   clName = cl.getCollectionName() ;
   cout<<"Before drop, the exist cl is : "<<clName<<endl ;
   rc = cs.dropCollection( COLLECTION_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cs.getCollection( COLLECTION_NAME, cl1 ) ;
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) ;
   cout<<"After drop, the cl named \"testbar\" does not exist."<<endl ;

   connection.disconnect() ;
}
/*
TEST(collectionspace,create)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCollection cl1 ;

   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   initEnv() ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_TRUE( 1==0 ) ;
   connection.disconnect() ;
}
*/

/*
TEST(collectionspace,drop)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCollection cl1 ;

   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   initEnv() ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_TRUE( 1==0 ) ;
   connection.disconnect() ;
}
*/

/*
TEST(collectionspace,createCollection_with_Sharding_and_replSize)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   initEnv() ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_TRUE( 0==1 ) ;
   connection.disconnect() ;
}

*/



/*
create //deprecated
drop   //deprecated
getCSName


*/
