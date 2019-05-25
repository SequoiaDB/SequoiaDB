#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;

TEST(cursor, not_connect)
{
   INT32 rc = SDB_OK ;
   sdbCursor cursor ;
   BSONObj obj ;

   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_NOT_CONNECTED, rc ) ;
}

TEST(cursor,current)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   rc = initEnv() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = insertRecords ( cl, 10 ) ;
   CHECK_MSG("%s%d","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor ) ;
   CHECK_MSG("%s%d","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.current( obj ) ;
   CHECK_MSG("%s%d","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"The current record is:"<<endl;
   cout<< obj.toString() <<endl ;
   connection.disconnect() ;
}

TEST(cursor,next)
{
   sdb connection ;
   sdbCollection cl ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;

   rc = initEnv() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = insertRecords ( cl, 10 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"The current record is:"<<endl;
   cout<< obj.toString() << endl ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"The next record is:"<<endl;
   cout<< obj.toString() << endl ;
   connection.disconnect() ;
}

TEST(cursor, next_next_close)
{
   sdb connection ;
   sdbCollection cl ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   INT32 NUM                                = 1 ;

   BSONObj obj ;
   BSONObj obj1 ;
   BSONObj obj2 ;

   rc = initEnv() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = insertRecords ( cl, NUM ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.next( obj ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"The current record is:"<<endl;
   cout<< obj.toString() << endl ;
   rc = cursor.next( obj1 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   rc = cursor.current( obj1 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   rc = cursor.next( obj2 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   rc = cursor.current( obj2 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   rc = cursor.close() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   connection.disconnect() ;
}


/*
TEST(cursor,updatecurrent)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;

   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   bson rule;
   bson obj;

   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = insertRecords ( cl, 10 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor, NULL, NULL, NULL, NULL, 0, -1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_init( &obj );
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"The current record is:"<<endl;
   bson_print( &obj ) ;
   bson_destroy( &obj );
   const char *r ="{$set:{id:\"count\",\
                          address:{streetAddress: \"21 2ndStreet\",\
                          city:\"NewYork\",state:\"NY\",postalCode:\"10021\"},\
                          phoneNumber:[{type: \"home\",number:\
                          \"212555-1234\"}]}}" ;
   bson_init( &rule );
   rc = jsonToBson ( &rule, r ) ;
   cout<<"The rule is :"<<endl ;
   bson_print( &rule ) ;
   ASSERT_EQ( TRUE, rc ) ;
   rc = cursor.updateCurrent( rule ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &rule );
   bson_init( &obj );
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"After update, current record is:"<<endl ;
   bson_print( &obj ) ;
   bson_destroy( &obj ) ;
   bson_init( &obj );
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"After update, next record is:"<<endl;
   bson_print( &obj ) ;
   bson_destroy( &obj );

   connection.disconnect() ;
}

TEST(cursor,deleteCurrent)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   bson obj;
   rc = initEnv();
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = insertRecords( cl, 10 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.query( cursor, NULL, NULL, NULL, NULL, 0, -1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_init( &obj );
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"Before delete, current record is: "<<endl ;
   bson_print( &obj ) ;
   bson_destroy( &obj );
   rc = cursor.delCurrent() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_init( &obj );
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_CURRENT_RECORD_DELETED, rc ) ;
   cout<<"After delete,the record no longer exist."<<endl ;
   bson_destroy( &obj );
   bson_init( &obj );
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"After delete,the next record is: "<<endl ;
   bson_print( &obj ) ;
   bson_destroy( &obj );
   connection.disconnect() ;
}
*/
