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
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   // initialize the work environment
   rc = initEnv() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert records
   rc = insertRecords ( cl, 10 ) ;
   CHECK_MSG("%s%d","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // query all the record in this collection
   rc = cl.query( cursor ) ;
   CHECK_MSG("%s%d","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the current record
   rc = cursor.current( obj ) ;
   CHECK_MSG("%s%d","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"The current record is:"<<endl;
   cout<< obj.toString() <<endl ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(cursor,next)
{
   sdb connection ;
   sdbCollection cl ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;

   // initialize the work environment
   rc = initEnv() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert records
   rc = insertRecords ( cl, 10 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // query all the record in this collection
   rc = cl.query( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the current record
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"The current record is:"<<endl;
   cout<< obj.toString() << endl ;
   // get the next record
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"The next record is:"<<endl;
   cout<< obj.toString() << endl ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(cursor, next_next_close)
{
   sdb connection ;
   sdbCollection cl ;
   sdbCollectionSpace cs ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   INT32 NUM                                = 1 ;

   BSONObj obj ;
   BSONObj obj1 ;
   BSONObj obj2 ;

   // initialize the work environment
   rc = initEnv() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert records
   rc = insertRecords ( cl, NUM ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // query all the record in this collection
   rc = cl.query( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the current record. expect rc == SDB_OK
   rc = cursor.next( obj ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"The current record is:"<<endl;
   cout<< obj.toString() << endl ;
   // get the next record, expect rc == SDB_DMS_EOC
   rc = cursor.next( obj1 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   // get current, expect rc == SDB_DMS_CONTEXT_IS_CLOSE
   rc = cursor.current( obj1 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   // get the next record, expect rc == SDB_DMS_CONTEXT_IS_CLOSE
   rc = cursor.next( obj2 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   // get  current, expect SDB_DMS_CONTEXT_IS_CLOSE
   rc = cursor.current( obj2 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) ;
   // close cursor, expect rc = SDB_OK
   rc = cursor.close() ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // disconnect the connection
   connection.disconnect() ;
}


/*
TEST(cursor,updatecurrent)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;

   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   bson rule;
   bson obj;

   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert records
   rc = insertRecords ( cl, 10 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // query all the record in this collection
   rc = cl.query( cursor, NULL, NULL, NULL, NULL, 0, -1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the current record
   bson_init( &obj );
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"The current record is:"<<endl;
   bson_print( &obj ) ;
   bson_destroy( &obj );
   // set updatecurrent rule
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
   // update current record
   rc = cursor.updateCurrent( rule ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &rule );
   // get the current record again
   bson_init( &obj );
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"After update, current record is:"<<endl ;
   bson_print( &obj ) ;
   bson_destroy( &obj ) ;
   //get the next record
   bson_init( &obj );
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"After update, next record is:"<<endl;
   bson_print( &obj ) ;
   bson_destroy( &obj );

   // disconnect the connection
   connection.disconnect() ;
}

TEST(cursor,deleteCurrent)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   bson obj;
   // initialize the word environment
   rc = initEnv();
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert records
   rc = insertRecords( cl, 10 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   //query all the record in this collection
   rc = cl.query( cursor, NULL, NULL, NULL, NULL, 0, -1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   //get the current record
   bson_init( &obj );
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"Before delete, current record is: "<<endl ;
   bson_print( &obj ) ;
   bson_destroy( &obj );
   //delete current record
   rc = cursor.delCurrent() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   //get the current record again
   bson_init( &obj );
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_CURRENT_RECORD_DELETED, rc ) ;
   cout<<"After delete,the record no longer exist."<<endl ;
   bson_destroy( &obj );
   //get the next record
   bson_init( &obj );
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"After delete,the next record is: "<<endl ;
   bson_print( &obj ) ;
   bson_destroy( &obj );
   // disconnect the connection
   connection.disconnect() ;
}
*/
