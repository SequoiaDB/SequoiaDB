#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;

#define INDEXNAMEDEF         "indexNameDef"

TEST(collection, not_connect)
{
   sdbCollection cl ;
   SINT64 count = 0 ;
   INT32 rc = SDB_OK ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_NOT_CONNECTED, rc ) ;
}

TEST(collection,getCount_without_condition)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   SINT64 count                             = 0 ;
   SINT64 NUM                               = 1000 ;

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
   // insert some record
   rc = insertRecords ( cl, NUM ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the count of record in currrent collection
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( NUM, count ) ;
   cout<<"The total number of records is "<<count<<endl ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,getCount_with_condition)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;

   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   SINT64 count                             = 0 ;
   BSONObj condition1 ;
   BSONObj condition2 ;
   BSONObj obj ;

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
   // build up the condition1 that "age is 25"
   condition1 = BSON ( "age" << 50 ) ;
   // get the count of specified record in currrent collection
   rc = cl.getCount( count, condition1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"Condition1 is"<<condition1<<endl ;
   cout<<"The number of matched records is "<<count<<endl ;
   // build up the condition2 that "age greater then 25"
   condition2 = BSON ( "age" << BSON ( "$gt" << 50 ) ) ;
   // get the count of specified record in currrent collection
   rc = cl.getCount( count, condition2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"Condition2 is"<<condition2<<endl ;
   cout<<"The number of records is "<<count<<endl ;

   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,bulkInsert)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   SINT64 NUM                               = 10000 ;
   SINT64 totalNum                          = 0 ;
   int count                                = 0 ;
   vector<BSONObj> objList ;
   BSONObj obj ;

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
   // count the total number of records before bulkInsert
   rc = cl.getCount (  totalNum ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf("Before bulk insert, the total number \
of records is %lld\n",totalNum ) ;
   printf( "Bulk insert records." OSS_NEWLINE ) ;
   // allocate memory and add data
   for ( count = 0; count < NUM; count++ )
   {
      obj = BSON ( "firstName" << "John" <<
                   "lastName" << "Smith" <<
                   "age" << 50 ) ;
      objList.push_back ( obj ) ;
   }
   // bulk insert,if the argument "flags" is set FLG_INSERT_CONTONDUP,
   // datebase will not stop bulk insert while one failed with dup key
   rc = cl.bulkInsert( 0, objList ) ;
   CHECK_MSG("%s%d\n", "rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // count the total number of records after insert
   rc = cl.getCount ( totalNum ) ;
   CHECK_MSG("%s%lld%s%lld%s%d\n", " NUM = ", NUM,
             " totalNum = ", totalNum, " rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( NUM, totalNum ) ;
   printf("After bulk insert,the total number \
of records is %lld\n",totalNum ) ;

   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,bulkInsert_empty)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   vector<BSONObj> objList ;
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
   // count the total number of records before bulkInsert
   rc = cl.bulkInsert( 0, objList ) ;
   CHECK_MSG("%s%d\n", "rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection, insert)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   bson::OID oid ;
   bson::OID oid2 ;
   BSONObj record = BSON( "_id" << 1 << "a" << 1 ) ;
   BSONObj rec_arr[3] ;
   rec_arr[0] = BSON( "_id" << 2 << "a" << 2 ) ;
   rec_arr[1] = BSON( "_id" << 3 << "a" << 3 ) ;
   rec_arr[2] = BSON( "_id" << 3 << "a" << 4 ) ;
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
   // insert
   rc = cl.insert( record, FLG_INSERT_CONTONDUP, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "oid is: " << oid.toString().c_str() << endl ;
   rc = cl.insert( record, FLG_INSERT_CONTONDUP, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "insert again oid is: " << oid2.toString().c_str() << endl ;

   // bulk insert
   rc = cl.insert( rec_arr, 3, FLG_INSERT_CONTONDUP ) ;
   CHECK_MSG("%s%d\n", "rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,insert_without_iterator)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;

   // initialize the work environment
   rc = initEnv() ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // build up the record to insert
   createEnglishRecord( obj ) ;
   cout<<"the insert record is(notice the id):"<<endl;
   cout<<toJson(obj)<<endl;
   // insert into collection
   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,insert_Chinese_record)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize the work environment

   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;

   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection, COLLECTION_SPACE_NAME, cs ) ;
   // get cl
   rc = getCollection ( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // build up the Chinese record to insert
   createChineseRecord( obj ) ;
   cout<<"the insert Chinese record is:"<<endl;
   cout << obj.toString () << endl ;
   // insert into collection
   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,update_condition_is_true)
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
   BSONElement ele ;
   BSONObj rule ;
   BSONObj obj ;
   BSONObjBuilder ob ;
   BSONObj updateCondition ;

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
   rc = insertRecords( cl, 10 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the current record
   cl.query(cursor) ;
   rc = cursor.current( obj ) ;
   CHECK_MSG("%s%d","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // pick up the updatecondition
   // for the use of updating
   ele = obj.getField ( "_id" ) ;
   ASSERT_TRUE( !ele.eoo() ) ;
   ob.append ( ele ) ;
   updateCondition = ob.obj () ;
   //set updatecurrent rule
   rule = BSON ( "$set" << BSON ( "age" << 19 ) ) ;
   cout<<"The update rule is:"<<endl;
   cout << rule.toString() << endl ;
   //update current record
   rc = cl.update( rule, updateCondition ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,update_condition_is_false)
{

   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc = SDB_OK ;
   BSONElement ele ;
   BSONObj rule ;
   BSONObj updateCondition ;

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
   // set updatecurrent rule
   rule = BSON ( "$set" << BSON ( "age" << 19 ) ) ;
   cout<<"the update rule is:"<<endl;
   cout << rule.toString() << endl ;
   // set update condition
   // the condition doesn't exit in any record
   updateCondition = BSON ( "condition" << "the condition doesn't exist in any record" ) ;
   rc = cl.update( rule, updateCondition ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,upsert_condition_is_true)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc = SDB_OK ;
   BSONElement ele ;
   BSONObj rule ;
   BSONObj obj ;
   BSONObj updateCondition ;
   BSONObjBuilder ob ;
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
   rc = insertRecords( cl, 10 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the current record
   sdbCursor cursor ;
      //first, need to query all the record
   cl.query(cursor) ;
      //second, get the current one
   rc = cursor.current(obj) ;
   ASSERT_EQ( SDB_OK, rc ) ;
      //then pick up the updatecondition
      //for the use of updating
   ele = obj.getField ( "_id" ) ;
   ASSERT_TRUE( !ele.eoo() ) ;
   ob.append ( ele ) ;
   updateCondition = ob.obj () ;
      //set updatecurrent rule
   rule = BSON ( "$set" << BSON ( "age" << 100 ) ) ;
   cout<<"the update rule is:"<<endl;
   cout << rule.toString() << endl ;
   //update current record
   rc = cl.upsert( rule, updateCondition ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,upsert_condition_is_false)
{

   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc = SDB_OK ;
   BSONElement ele ;
   BSONObj rule ;
   BSONObj obj ;
   BSONObj updateCondition ;

   // initialize the work environment
   rc = initEnv() ;
   // connect to database

   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // set updatecurrent rule
   rule = BSON ( "$set" << BSON ( "age" << 19 ) ) ;
   cout<<"the update rule is:"<<endl;
   cout << rule.toString() << endl ;
   // set update condition
   // the condition doesn't exit in any record
   updateCondition = BSON ( "condition" << "the condition doesn't exist in any record" ) ;
   rc = cl.upsert( rule, updateCondition ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,del_without_condition)
{

   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc = SDB_OK ;

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
   // set delete all the record in current collection
   rc = cl.del() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,del_with_condition)
{

   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc = SDB_OK ;
   BSONObj condition ;
   // initialize the work environment
   rc = initEnv() ;
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
   // build up the delete condition
   condition  = BSON ( "age" << 50 ) ;
   // delete the specified records in current collection
   rc = cl.del( condition ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,query)
{

   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc = SDB_OK ;

   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj condition ;
   // connect to database
   sdbclient::sdb connection ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   sdbclient::sdbCollectionSpace cs ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   sdbclient::sdbCollection cl ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // define a cursor object for query
   sdbclient::sdbCursor cursor ;
   // build up the query condition
   condition = BSON ( "age" << 50 ) ;
   // query the specified records in current collection
   rc = cl.query( cursor, condition  ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection, query_with_flags)
{
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc = SDB_OK ;

   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj obj ;
   sdbclient::sdb connection ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbclient::sdbCollectionSpace cs ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbclient::sdbCollection cl ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbclient::sdbCursor cursor ;

   // flag is 0
   rc = cl.query( cursor, obj, obj, obj, obj, 0, -1, 0 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // flag
   rc = cl.query( cursor, obj, obj, obj, obj, 0, -1, QUERY_FORCE_HINT ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // flag
   rc = cl.query( cursor, obj, obj, obj, obj, 0, -1, QUERY_WITH_RETURNDATA ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // flag
   rc = cl.query( cursor, obj, obj, obj, obj, 0, -1,
                  QUERY_PARALLED | QUERY_WITH_RETURNDATA ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // flag
   rc = cl.query( cursor, obj, obj, obj, obj, 0, -1,
                  QUERY_WITH_RETURNDATA | QUERY_FORCE_HINT | QUERY_PARALLED ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   connection.disconnect() ;
}

TEST(collection,queryOne)
{
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc = SDB_OK ;
   INT32 record_num = 100 ;
   BSONObj cond ;
   BSONObj sel ;
   BSONObj order ;
   BSONObj record ;
   BSONObj dumpObj ;

   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   sdbclient::sdb connection ;
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   sdbclient::sdbCollectionSpace cs ;
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   sdbclient::sdbCollection cl ;
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert some records
   rc = insertRecords( cl, record_num ) ;
   // define a cursor object for query
   sdbclient::sdbCursor cursor ;
   // build up the query condition
   cond = BSON ( "age" << 50 ) ;
   sel = BSON( "age" << "" ) ;
   order = BSON( "age" << -1 ) ;
   // query the specified records in current collection
   rc = cl.queryOne( record, cond, sel, order,
                     dumpObj, record_num - 1, 0 ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "queryOne result is: " << record.toString( FALSE, TRUE ) << endl ;
   cond = BSON( "age" << 51 ) ;
   rc = cl.queryOne( record, cond, sel, order,
                     dumpObj, record_num - 1, 0 ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,createIndex)
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
   const CHAR *pIndexName1                  = "indexNameDef" ;
   const CHAR *pIndexName2                  = "index_cpp_offline" ;
   const CHAR *pStr                         = NULL ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   BSONElement ele ;
   BSONObj tmp_obj ;

   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs );
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   /// createIndex
   // build a bson for index definition
   obj = BSON ( "name" << 1 << "age" << -1 ) ;
   // create index
   rc = cl.createIndex( obj, pIndexName1, TRUE, TRUE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // get the newly build index
   rc = cl.getIndexes( cursor, pIndexName1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // print the index record
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "After creating index in online mode, the current index is:\n" ) ;
   cout << obj.toString() << endl ;
   ele = obj.getField( "IndexDef" ) ;
   if ( Object != ele.type() )
   {
      ASSERT_EQ( 0, 1 ) << "invalid object" ;
   }
   tmp_obj = ele.embeddedObject().getOwned() ;
   ele = tmp_obj.getField( "name" ) ;
   pStr = ele.valuestr() ;
   ASSERT_EQ( 0, strncmp( pStr, pIndexName1, sizeof(pIndexName1) ) ) ;

   /// createIndexOffline
   obj = BSON ( "name2" << 1 << "age2" << -1 ) ;
   // create index
   rc = cl.createIndex( obj, pIndexName2, TRUE, TRUE, 100 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // get the newly build index
   rc = cl.getIndexes( cursor, pIndexName2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // print the index record
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "After creating index in offline mode, the current index is:\n" ) ;
   cout << obj.toString() << endl ;
   ele = obj.getField( "IndexDef" ) ;
   if ( Object != ele.type() )
   {
      ASSERT_EQ( 0, 1 ) << "invalid object" ;
   }
   tmp_obj = ele.embeddedObject().getOwned() ;
   ele = tmp_obj.getField( "name" ) ;
   pStr = ele.valuestr() ;
   ASSERT_EQ( 0, strncmp( pStr, pIndexName2, sizeof(pIndexName2) ) ) ;

   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,getIndexes)
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
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs );
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // get the index
   rc = cl.getIndexes( cursor, "$id" ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // print the index record
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "The current index we get is:\n" ) ;
   cout << obj.toString() << endl ;

   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,dropIndex)
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
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs );
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   obj = BSON ( "name" << 1 << "age" << -1 ) ;
   // build a bson for index definition
   // create index
   rc = cl.createIndex( obj, INDEXNAMEDEF, FALSE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // drop index
   rc = cl.dropIndex( INDEX_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the index again
   rc = cl.getIndexes( cursor, INDEX_NAME ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // print the index record
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) ;

   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,getCollectionName)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *clName                       = NULL ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // get the collection name
   clName = cl.getCollectionName() ;
   // print the cl name
   cout<<"The cl name is ："<<clName<<endl ;
   ASSERT_EQ( 0, strncmp( clName, COLLECTION_NAME, sizeof(COLLECTION_NAME) ) ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,getCSName)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *csName                       = NULL ;
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

   // get the collection name
   csName = cl.getCSName() ;
   // print the cl name
   cout<<"The cs name is ："<<csName<<endl ;
   ASSERT_EQ( 0, strncmp( csName, COLLECTION_SPACE_NAME,
                          sizeof(COLLECTION_SPACE_NAME) ) ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,getFullName)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   const CHAR *fullName                       = NULL ;
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

   // get the collection name
   fullName = cl.getFullName() ;
   // print the cl name
   cout<<"The full name is ："<<fullName<<endl ;
   ASSERT_EQ( 0, strncmp( fullName, COLLECTION_FULL_NAME,
                          sizeof(COLLECTION_FULL_NAME) ) );
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,aggregate)
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
   vector<BSONObj> ob ;
   int iNUM = 2 ;
   int rNUM = 4 ;
   int i = 0 ;
   const char* command[iNUM] ;
   const char* record[rNUM] ;
   command[0] = "{$match:{status:\"A\"}}" ;
   command[1] = "{$group:{_id:\"$cust_id\",total:{$sum:\"$amount\"}}}" ;
   record[0] = "{cust_id:\"A123\",amount:500,status:\"A\"}" ;
   record[1] = "{cust_id:\"A123\",amount:250,status:\"A\"}" ;
   record[2] = "{cust_id:\"B212\",amount:200,status:\"A\"}" ;
   record[3] = "{cust_id:\"A123\",amount:300,status:\"D\"}" ;
   const char* m = "{$match:{status:\"A\"}}" ;
   const char* g = "{$group:{_id:\"$cust_id\",total:{$sum:\"$amount\"}}}" ;

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

   // insert record
   for( i=0; i<rNUM; i++ )
   {
      rc = fromjson( record[i], obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      cout<<obj.toString()<<endl ;
      rc = cl.insert( obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   // build bson vector
   for ( i=0; i<iNUM; i++ )
   {
      rc = fromjson( command[i], obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      cout<<obj.toString()<<endl ;
      ob.push_back( obj ) ;
   }
   // aggregate
   rc = cl.aggregate( cursor, ob ) ;
   cout<<"rc is "<<rc<<endl ;
   // display
   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection, aggregate_2)
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
   vector<BSONObj> ob ;
   int iNUM = 5 ;
   int rNUM = 5 ;
   int i = 0 ;
   const char* command[iNUM] ;
   const char* record[rNUM] ;
   command[0] = "{\"$match\":{\"interest\":{\"$exists\":1}}}" ;
   command[1] = "{\"$group\":{\"_id\":\"$major\",\"avg_age\":{\"$avg\":\"$info.age\"},\"major\":{\"$first\":\"$major\"}}}" ;
   command[2] = "{\"$sort\":{\"avg_age\":-1,\"major\":1}}" ;
   command[3] = "{\"$skip\":0}" ;
   command[4] = "{\"$limit\":5}" ;

   record[0] = "{\"no\":1000,\"score\":80,\"interest\":[\"basketball\",\"football\"],\"major\":\"computer th\",\"dep\":\"computer\",\"info\":{\"name\":\"tom\",\"age\":25,\"gender\":\"man\"}}" ;
   record[1] = "{\"no\":1001,\"score\":90,\"interest\":[\"basketball\",\"football\"],\"major\":\"computer sc\",\"dep\":\"computer\",\"info\":{\"name\":\"mike\",\"age\":24,\"gender\":\"lady\"}}" ;
   record[2] = "{\"no\":1002,\"score\":85,\"interest\":[\"basketball\",\"football\"],\"major\":\"computer en\",\"dep\":\"computer\",\"info\":{\"name\":\"kkk\",\"age\":25,\"gender\":\"man\"}}" ;
   record[3] = "{\"no\":1003,\"score\":92,\"interest\":[\"basketball\",\"football\"],\"major\":\"computer en\",\"dep\":\"computer\",\"info\":{\"name\":\"mmm\",\"age\":25,\"gender\":\"man\"}}" ;
   record[4] = "{\"no\":1004,\"score\":88,\"interest\":[\"basketball\",\"football\"],\"major\":\"computer sc\",\"dep\":\"computer\",\"info\":{\"name\":\"ttt\",\"age\":25,\"gender\":\"man\"}}" ;

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

   // insert record
   for( i=0; i<rNUM; i++ )
   {
      rc = fromjson( record[i], obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      cout<<obj.toString()<<endl ;
      rc = cl.insert( obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   // build bson vector
   for ( i=0; i<iNUM; i++ )
   {
      rc = fromjson( command[i], obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      cout<<obj.toString()<<endl ;
      ob.push_back( obj ) ;
   }
   // aggregate
   rc = cl.aggregate( cursor, ob ) ;
   cout<<"rc is "<<rc<<endl ;
   // display
   displayRecord( cursor ) ;
   // disconnect the connection
   connection.disconnect() ;
}


TEST( collection, getQueryMeta )
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor datacursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   BSONObj temp ;
   BSONObj empty ;
   BSONObj condition ;
   BSONObj orderBy ;
   BSONObj hint ;
   string st ;
   const char* st1 = "tbscan" ;
   const char* st2 = "ixscan" ;
   long i = 0 ;

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
   // condition
   condition = BSON( "age"<<BSON("$gt" << 0)<<"age"<<BSON( "$lt" << 100 ) ) ;
   cout<<"condition is: "<<condition.toString()<<endl ;
   // hint
   int flag = 0 ;
   if( 0 == flag )
   {
      hint = BSON( "" << "ageIndex" ) ;
   }
   else
   {
      BSONObjBuilder ob1 ;
      ob1.appendNull ( "" ) ;
      hint = ob1.obj() ;
   }
   // orderBy
   orderBy = BSON( "Indexblocks"<<1 ) ;
   // TO DO:
   rc = cl.getQueryMeta( cursor, condition, empty, hint, 0, -1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   while ( !( rc = cursor.next( obj ) ) )
   {
      cout<<obj.toString()<<endl ;
      BSONObjBuilder bo ;
      obj.getField( "ScanType" ).Val( st ) ;
      cout<<"ScanType is: "<<st.c_str()<<endl ;
      if ( !st.compare( st1 ) )
      {
         bo.appendAs( obj.getField( "Datablocks" ), "Datablocks" ) ;
      }
      else if ( !st.compare( st2 ) )
      {
         bo.appendAs( obj.getField( "Indexblocks" ), "Indexblocks" ) ;
      }
      else
      {
         cout<<"the \"ScanType\" is "<<st.c_str()
             <<",not \"tbscan\" or \"ixscan\"."<<endl ;
         ASSERT_EQ( 1, 0 ) ;
      }
      BSONObj hint = bo.obj() ;
      cout<<"hint is: "<<hint.toString()<<endl ;
      rc = cl.query( datacursor, empty, empty, empty, hint, 0, -1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      while ( !( rc = datacursor.next( temp ) ) )
      {
//         cout<<temp.toString()<<endl ;
         i++ ;
      }
   }
   cout<<"the record number is: "<<i<<endl ;
   ASSERT_EQ( 1, 1 ) ;
   connection.disconnect() ;

}

TEST( collection, getQueryMeta_select_is_null )
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   sdbCursor datacursor ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   BSONObj temp ;
   BSONObj empty ;
   BSONObj condition ;
   BSONObj orderBy ;
   BSONObj hint ;
   BSONObjBuilder ob1 ;
   string st ;
   const char* st1 = "tbscan" ;
   const char* st2 = "ixscan" ;
   long i = 0 ;

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
   // build condition
   condition = BSON( "age"<<BSON("$gt" << 50) ) ;
   cout<<"condition is: "<<condition.toString()<<endl ;
   // select
   int flag = 1 ;
   if( 0 == flag )
   {
      hint = BSON( "" << "ageIndex" ) ;
   }
   else
   {
      ob1.appendNull ( "" ) ;
      hint = ob1.obj() ;
   }
   // orderBy
   orderBy = BSON( "Datablocks"<<1 ) ;
   // TO DO:
   rc = cl.getQueryMeta( cursor, condition, empty, hint, 0, -1 ) ;
   CHECK_MSG( "%s%d\n", "rc = ", rc ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   while ( !( rc = cursor.next( obj ) ) )
   {
      cout<<obj.toString()<<endl ;
      BSONObjBuilder bo ;
      obj.getField( "ScanType" ).Val( st ) ;
      cout<<"ScanType is: "<<st.c_str()<<endl ;
      if ( !st.compare( st1 ) )
      {
         bo.appendAs( obj.getField( "Datablocks" ), "Datablocks" ) ;
      }
      else if ( !st.compare( st2 ) )
      {
         bo.appendAs( obj.getField( "Indexblocks" ), "Indexblocks" ) ;
      }
      else
      {
         cout<<"the \"ScanType\" is "<<st.c_str()
             <<",not \"tbscan\" or \"ixscan\"."<<endl ;
         ASSERT_TRUE( 1 == 0 ) ;
      }
      BSONObj hint = bo.obj() ;
      cout<<"hint is: "<<hint.toString()<<endl ;
      rc = cl.query( datacursor, empty, empty, empty, hint, 0, -1 ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      while ( !( rc = datacursor.next( temp ) ) )
      {
//         cout<<temp.toString()<<endl ;
         i++ ;
      }
   }
   cout<<"the record number is: "<<i<<endl ;
   ASSERT_TRUE( 1==1 ) ;
   connection.disconnect() ;

}

TEST(collection, attachCollection)
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
   INT32 NUM                                = 100 ;
   INT32 i                                  = 0 ;
   SINT64 count                             = 0 ;
   BSONObj obj ;
   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // test whether it is in the cluser environment, because
   // 'SDB_SNAP_CATALOG' could not use in standalone
   rc = connection.getList( cursor, SDB_LIST_GROUPS ) ;
   if ( rc == SDB_RTN_COORD_ONLY )
   {
      cout << "attachCollection use \
in the cluser environment only" << endl ;
      ASSERT_EQ( SDB_RTN_COORD_ONLY, rc );
      return ;
   }
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // create main cl
   rc = cs.createCollection( "main",
                             BSON("IsMainCL"<<true<<"ShardingKey"<<BSON("id"<<1)),
                             cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // attach the sub collection "testbar"
   obj =  BSON( "LowBound" << BSON("id"<<0) << "UpBound" << BSON("id"<<100) ) ;
   rc = cl.attachCollection ( COLLECTION_FULL_NAME, obj ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert some data for test
   for ( i=0; i < NUM; i++ )
   {
      rc = cl.insert(BSON("id"<<i)) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   INT32 cnt = 0 ;
   do
   {
      rc = cl.getCount ( count ) ;
      ++cnt ;
   }while( 100 != count && 100 > cnt ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "NUM is: " << NUM << "count is: " << count << endl ;
   ASSERT_EQ( NUM, count ) ;
   // detach
   rc = cl.detachCollection ( COLLECTION_FULL_NAME ) ;
   CHECK_MSG("%s%d\n","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // after detach, use a invalid record to test again
   rc = cl.insert(BSON("id"<<101)) ;
   cout << "rc is: " << rc << endl ;
   ASSERT_EQ( SDB_CAT_NO_MATCH_CATALOG, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}



/* some doubtful test */
/*
TEST(collection,insert_with_iterator)
{
   sdb connection ;
   sdbCollection collection ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   BSONElement ele ;
   BSONObj obj ;
   const char *key                          = NULL ;
   const char *val                          = NULL ;
   int value                                = 5 ;

   // initialize the work environment
   initEnv() ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get collection
   rc = getCollection( connection, COLLECTION_FULL_NAME, collection ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // build the record to insert
   obj = BSON ( "a" << 1 ) ;
   cout<<"The insert record is :"<<endl;
   cout << obj.toString() << endl ;
   // insert into collection
   rc = collection.insert( obj, &ele ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"Read the record by the iterator:"<<endl;
//   bson_iterator_next( &it ) ;
   //key = bson_iterator_key( &it ) ;
   //val = bson_iterator_string( &it ) ;
//   value = bson_iterator_int( &it ) ;
   //printf("The insert record is {%s:%d}\n", key, value ) ;
   //printf("The insert record is {%s:%s}\n", key, val ) ;
//   cout<<"{"<<key<<":"
  //     <<value<<"}"<<endl ;

   //ASSERT_EQ( 1, 0 ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection,create)
{
   ASSERT_TRUE( 0==0) ;
}

TEST(collection,drop)
{
   ASSERT_TRUE( 0==0) ;
}

*/

/*******************************************************************************
*@Description : query one testcase.[db.foo.bar.findOne()]
*@Modify List :
*               2014-10-24   xiaojun Hu   Init
*******************************************************************************/
TEST( collection, sdbCppQueryOne )
{
/*
   INT32 rc = SDB_OK ;

   sdbclient::sdb db ;
   rc = db.connect( HOST, SERVER, USER, PASSWD ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
*/
}

TEST(collection, truncate)
{
   sdb connection ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   INT32 rc                                 = SDB_OK ;
   INT32 i                                  = 0 ;
   SINT64 count                             = 0 ;
   SINT64 NUM                               = 100 ;

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
   // insert some record
   rc = insertRecords ( cl, NUM ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // test
   rc = cl.truncate() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( 0, count ) ;
   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection, queryAndUpdate)
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
   const CHAR *pField1                      = "testField1" ;
   const CHAR *pField2                      = "testField2" ;
   const CHAR *pIndexName1                  = "testIndex1" ;
//   const CHAR *pIndexName2                  = "testIndex2" ;
   INT32 rc                                 = SDB_OK ;
   INT32 i                                  = 0 ;
   SINT64 count                             = 0 ;
   SINT64 NUM                               = 100 ;
   BSONObj update ;
   BSONObj condition ;
   BSONObj selector ;
   BSONObj orderBy ;
   BSONObj hint ;

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

   // create index
   BSONObj index = BSON( pField1 << 1 ) ;
   rc = cl.createIndex( index, pIndexName1, FALSE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // insert some record
   for ( i = 0; i < NUM; i++ )
   {
      BSONObj obj = BSON( pField1 << i << pField2 << i ) ;
      rc = cl.insert( obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   update = BSON( "$set" << BSON( pField2 << 100 ) ) ;
   condition = BSON( pField1 << BSON( "$gte" << 0 ) ) ;
   selector = BSON( pField2 << "" ) ;
   orderBy = BSON( pField1 << -1 ) ;
   hint = BSON( "" << pIndexName1 ) ;

   // test
   rc = cl.queryAndUpdate( cursor, update, condition, selector,
                           orderBy, hint, 0, -1, 0, TRUE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check
   BSONObj obj ;
   i = 0 ;
   while( SDB_OK == cursor.next( obj ) )
   {
      i++ ;
      BSONObjIterator it( obj ) ;
      BSONElement ele = it.next() ;
      INT32 num = ele.Int() ;
      ASSERT_EQ( 100, num ) ;
   }
   ASSERT_EQ( 100, i ) ;

   // disconnect the connection
   connection.disconnect() ;
}

TEST(collection, queryAndRemove)
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
   const CHAR *pField1                      = "testField1" ;
   const CHAR *pField2                      = "testField2" ;
   const CHAR *pIndexName1                  = "testIndex1" ;
   const CHAR *pIndexName2                  = "testIndex2" ;
   INT32 rc                                 = SDB_OK ;
   INT32 i                                  = 0 ;
   SINT64 count                             = 0 ;
   SINT64 NUM                               = 100 ;
   BSONObj condition ;
   BSONObj selector ;
   BSONObj orderBy ;
   BSONObj hint ;
   BSONObj hint2 ;

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

   // create index
   BSONObj index = BSON( pField1 << 1 ) ;
   rc = cl.createIndex( index, pIndexName1, FALSE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObj index2 = BSON( pField2 << 1 ) ;
   rc = cl.createIndex( index2, pIndexName2, FALSE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // insert some record
   for ( i = 0; i < NUM; i++ )
   {
      BSONObj obj = BSON( pField1 << i << pField2 << i ) ;
      rc = cl.insert( obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   condition = BSON( pField1 << BSON( "$gte" << 0 ) ) ;
   selector = BSON( pField2 << "" ) ;
   orderBy = BSON( pField1 << -1 ) ;
   hint = BSON( "" << pIndexName1 ) ;
   hint2 = BSON( "" << pIndexName2 ) ;

   BSONObj tmp ;

   // test
   // case 1: use extend sort
   rc = cl.queryAndRemove( cursor, condition, selector,
                           orderBy, hint2, 0, -1, 0 ) ;
   ASSERT_EQ( SDB_RTN_QUERYMODIFY_SORT_NO_IDX, rc ) ;

   // case 2: does not use extend sort
   rc = cl.queryAndRemove( cursor, condition, selector,
                           orderBy, hint, 0, -1, 0 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // check
   BSONObj obj ;
   i = 0 ;
   while( SDB_OK == cursor.next( obj ) )
   {
      i++ ;
      BSONObjIterator it( obj ) ;
      BSONElement ele = it.next() ;
      INT32 num = ele.Int() ;
      ASSERT_EQ( 100 - i, num ) ;
   }
   ASSERT_EQ( 100, i ) ;
   i = 100 ;
   while( i-- )
   {
      rc = cl.getCount( count ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      if ( 0 == count )
         break ;
   }
   if ( 0 == i )
   {
      ASSERT_EQ( 0, count ) ;
   }

   // disconnect the connection
   connection.disconnect() ;
}

TEST( collection, alter_collection )
{
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   // initialize the work environment

   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   const CHAR *pCSName                      = "test_alter_cs" ;
   const CHAR *pCLName                      = "test_alter_cl" ;
   const CHAR *pCLFullName                  = "test_alter_cs.test_alter_cl" ;
   const CHAR *pValue                       = NULL ;

   INT32 rc                                 = SDB_OK ;
   BSONObjBuilder bob ;
   BSONObjBuilder bob2 ;
   BSONElement ele ;
   BSONObj option ;
   BSONObj matcher ;
   BSONObj record ;
   BSONObj obj ;
   INT32 n_value = 0 ;

   bob.append( "Name", pCLFullName ) ;
   matcher = bob.obj() ;

   bob2.append( "ReplSize", 0 ) ;
   bob2.append( "ShardingKey", BSON( "a" << 1 ) ) ;
   bob2.append( "ShardingType", "hash" ) ;
   bob2.append( "Partition", 1024 ) ;
   option = bob2.obj() ;

   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;


   // connect to database
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   if ( FALSE == isCluster( db ) )
   {
      return ;
   }

   // drop cs
   rc = db.dropCollectionSpace( pCSName ) ;
   if ( SDB_OK != rc && SDB_DMS_CS_NOTEXIST != rc )
   {
      ASSERT_EQ( 0, 1 ) << "failed to drop cs " << pCSName ;
   }

   // create cs and cl
   rc = db.createCollectionSpace( pCSName, 4096, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cs.createCollection( pCLName, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // alter
   rc = cl.alterCollection( option ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sleep( 1 ) ;

   // check
   rc = db.getSnapshot( cursor, SDB_SNAP_CATALOG, matcher ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cursor.next( record ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ele = record.getField( "Name" ) ;
   ASSERT_EQ( String, ele.type() ) << "bson element is not a string type" ;
//   pValue = ele.String().c_str() ;
   pValue = ele.valuestr() ;
   ASSERT_EQ( 0, strcmp( pValue, pCLFullName ) ) << "after alter cl, the cl's name is not what we want" ;

   ele = record.getField( "ReplSize" ) ;
   n_value = ele.Int() ;
   ASSERT_EQ( 7, n_value ) << "after alter cl, replSize is not 0" ;

   ele = record.getField( "ShardingKey" ) ;
   obj = ele.Obj() ;
   ele = obj.getField( "a" ) ;
   n_value = ele.Int() ;
   ASSERT_EQ( 1, n_value ) << "after alter cl, the sharding key is not what we want" ;

   ele = record.getField( "ShardingType" ) ;
   pValue = ele.valuestr() ;
   ASSERT_EQ( SDB_OK, strcmp( pValue, "hash") ) << "after alter cl, the sharding type is not what we want" ;
   ele = record.getField( "Partition" ) ;
   n_value = ele.Int() ;
   ASSERT_EQ( 1024, n_value ) ;

   rc = db.dropCollectionSpace( pCSName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   db.disconnect() ;
}

TEST( collection, create_remove_id_index )
{
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;
   // initialize the work environment

   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   const CHAR *pCLFullName                  = "" ;
   const CHAR *pIndexName                   = "$id" ;
   INT32 rc                                 = SDB_OK ;
   INT32 count                              = 0 ;
   BSONObj obj ;
   BSONObj record ;
   BSONObj updater ;
   BSONObj options ;

   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace ( db, COLLECTION_SPACE_NAME, cs ) ;
   // get cl
   rc = getCollection ( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // build record
   obj = BSON( "a" << 1 ) ;
   updater = BSON( "$set" << BSON( "a" << 2 ) ) ;

   // insert into collection
   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // remove $id index
   rc = cl.dropIdIndex() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // check
   rc = cl.getIndexes( cursor, pIndexName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;


   while( SDB_OK == (rc = cursor.next( record ) ) )
   {
      count++ ;
   }
   ASSERT_EQ( 0, count ) << "Index $id may not be drop" ;

   rc = cl.query( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   count = 0 ;
   while( SDB_OK == (rc = cursor.next( record ) ) )
   {
      count++ ;
   }
   ASSERT_EQ( 1, count ) ;

   rc = cl.update( updater ) ;
   ASSERT_EQ( SDB_RTN_AUTOINDEXID_IS_FALSE, rc ) ;

   rc = cl.upsert( updater ) ;
   ASSERT_EQ( SDB_RTN_AUTOINDEXID_IS_FALSE, rc ) ;

   rc = cl.del() ;
   ASSERT_EQ( SDB_RTN_AUTOINDEXID_IS_FALSE, rc ) ;

   // create $id index
   options = BSON( "Offline" << true ) ;
   rc = cl.createIdIndex( options ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.del() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   count = 0 ;
   while( SDB_OK == (rc = cursor.next( record ) ) )
   {
      count++ ;
   }
   ASSERT_EQ( 0, count ) ;

   obj = BSON( "$set" << BSON( "a" << 10 ) ) ;
   rc = cl.upsert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.query( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   count = 0 ;
   while( SDB_OK == (rc = cursor.next( record ) ) )
   {
      count++ ;
   }
   ASSERT_EQ( 1, count ) ;

   rc = cl.update( updater ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   obj = BSON( "a" << 2 ) ;
   rc = cl.query( cursor, obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   count = 0 ;
   while( SDB_OK == (rc = cursor.next( record ) ) )
   {
      count++ ;
   }
   ASSERT_EQ( 1, count ) ;


   // disconnect the connection
   db.disconnect() ;
}

TEST( collection, bson_timestamp_test )
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
   const CHAR *expect = "{ \"ts1\": {\"$timestamp\": \"2018-08-22-20.51.45.000000\"}, \"ts2\": {\"$timestamp\": \"2018-08-22-20.51.45.000001\"}, \"ts3\": {\"$timestamp\": \"2018-08-22-20.53.48.456789\"}, \"ts4\": {\"$timestamp\": \"1921-05-12-19.10.18.456789\"} }" ;
   INT32 rc                                 = SDB_OK ;
   INT64 ms = 1534942305000 ;
   BSONObj obj ;
   BSONObjBuilder bob ;
   bob.appendTimestamp( "ts1", ms, 0 ) ;
   bob.appendTimestamp( "ts2", ms, 1 ) ;
   bob.appendTimestamp( "ts3", ms, 123456789 ) ;
   bob.appendTimestamp( "ts4", -ms, 123456789 ) ;
   obj = bob.obj() ;
   BSONObj sel =  BSON( "_id" << BSON("$include" << 0) ) ;
   BSONObj empty ;
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
   rc = cl.insert( obj ) ;
   CHECK_MSG("%s%d","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // query all the record in this collection
   rc = cl.query( cursor, empty, sel ) ;
   CHECK_MSG("%s%d","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get the current record
   rc = cursor.current( obj ) ;
   CHECK_MSG("%s%d","rc = ",rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout<<"The current record is:"<<endl;
   cout<< obj.toString() <<endl ;
   ASSERT_EQ( 0, strncmp( expect, obj.toString().c_str(), strlen(expect) ) ) ;
   // disconnect the connection
   connection.disconnect() ;
}

void insertWithResultTest( sdbCollection &cl )
{
   INT32 rc = SDB_OK ;

   BSONObj record1 = BSON( "_id" << 110 << "a" << 1 ) ;
   BSONObj record2 = BSON( "a" << 2 ) ;
   BSONObj record3 = BSON( "a" << 2 ) ;
   BSONObj rec_arr1[3] ;
   BSONObj rec_arr2[3] ;
   BSONObj rec_arr3[3] ;
   rec_arr1[0] = BSON( "_id" << 52 << "a" << 3 ) ;
   rec_arr1[1] = BSON( "_id" << 53 << "a" << 4 ) ;
   rec_arr1[2] = BSON( "_id" << 54 << "a" << 5 ) ;

   rec_arr2[0] = BSON( "a" << 13 ) ;
   rec_arr2[1] = BSON( "a" << 14 ) ;
   rec_arr2[2] = BSON( "a" << 15 ) ;

   rec_arr3[0] = BSON( "_id" << 22 << "a" << 3 ) ;
   rec_arr3[1] = BSON( "_id" << 23 << "a" << 4 ) ;
   rec_arr3[2] = BSON( "_id" << 24 << "a" << 5 ) ;
   BSONObj result ;
   const CHAR *pStr = NULL ;

   printf( "in insert with flags\n" ) ;
   // initialize the work environment
   // insert
   rc = cl.insert( record1, FLG_INSERT_RETURN_OID, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "result is: " << result.toString().c_str() << endl ;
   pStr = "{ \"_id\": 110 }" ;
   ASSERT_EQ( 0, strncmp( pStr, result.toString().c_str(), strlen( pStr ) ) ) ;

   rc = cl.insert( record2, FLG_INSERT_CONTONDUP, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "result is: " << result.toString().c_str() << endl ;
   pStr = "{}" ;
   ASSERT_EQ( 0, strncmp( pStr, result.toString().c_str(), strlen( pStr ) ) ) ;

   rc = cl.insert( record3, FLG_INSERT_CONTONDUP | FLG_INSERT_RETURN_OID, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "insert again result is: " << result.toString().c_str() << endl ;

   rc = cl.insert( record3, FLG_INSERT_CONTONDUP | FLG_INSERT_RETURN_OID, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   result = BSON( "aa" << 0 ) ;
   rc = cl.insert( record3, 0, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // bulk insert
   rc = cl.insert( rec_arr1, 3, FLG_INSERT_RETURN_OID | FLG_INSERT_CONTONDUP, &result ) ;
   CHECK_MSG("%s%d\n", "rc = ", rc) ;
   cout << "result is: " << result.toString().c_str() << endl ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.insert( rec_arr2, 3, FLG_INSERT_CONTONDUP, &result ) ;
   CHECK_MSG("%s%d\n", "rc = ", rc) ;
   cout << "result is: " << result.toString().c_str() << endl ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.insert( rec_arr3, 3  ) ;
   CHECK_MSG("%s%d\n", "rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.insert( rec_arr3, 3, FLG_INSERT_RETURN_OID | FLG_INSERT_CONTONDUP ) ;
   CHECK_MSG("%s%d\n", "rc = ", rc) ;
   ASSERT_EQ( SDB_OK, rc ) ;


   rc = cl.insert( rec_arr3, 3, FLG_INSERT_RETURN_OID | FLG_INSERT_CONTONDUP, &result ) ;
   CHECK_MSG("%s%d\n", "rc = ", rc) ;
   cout << "result is: " << result.toString().c_str() << endl ;
   pStr = "{ \"_id\": [ 22, 23, 24 ] }" ;
   ASSERT_EQ( SDB_OK, rc ) ;

}

void insertTest( sdbCollection &cl )
{
   INT32 rc = SDB_OK ;

   const CHAR *pOid = "123456789012345678901234" ;
   bson::OID oid( pOid ) ;
   bson::OID oid0 ;
   bson::OID oid1 ;
   bson::OID oid2 ;
   BSONObj record = BSON( "_id" << oid << "a" << 0 ) ;
   BSONObj record0 = BSON( "_id" << 100 << "a" << 0 ) ;
   BSONObj record1 = BSON( "_id" << 1 << "a" << 1 ) ;
   vector<BSONObj> vec ;
   vec.push_back( BSON( "_id" << 10 << "a" << 3 ) ) ;
   vec.push_back( BSON( "_id" << 11 << "a" << 4 ) ) ;
   vec.push_back( BSON( "_id" << 11 << "a" << 5 ) ) ;
   const CHAR *pStr = NULL ;

   // initialize the work environment
   // insert
   printf( "in insert and bulk insert\n" ) ;
   rc = cl.insert( record, &oid0 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "oid is: " << oid0.toString().c_str() << endl ;
   ASSERT_EQ( 0, strncmp( pOid, oid0.toString().c_str(), strlen( pOid ) ) ) ;

   rc = cl.insert( record0, &oid0 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "oid is: " << oid0.toString().c_str() << endl ;
   pStr = "000000000000000000000000" ;
   ASSERT_EQ( 0, strncmp( pStr, oid0.toString().c_str(), strlen( pStr ) ) ) ;

   rc = cl.insert( record1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.bulkInsert( FLG_INSERT_CONTONDUP, vec ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

}

TEST(debug, getIndexes)
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
   const CHAR *pIndexName                   = "aIndex" ;
   BSONObj obj ;
   vector<BSONObj> vec ;

   // initialize the work environment
   rc = initEnv() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // connect to database
   rc = connection.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cs
   rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs );
   ASSERT_EQ( SDB_OK, rc ) ;
   // get cl
   rc = getCollection( cs, COLLECTION_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = cl.createIndex( BSON( "a" << 1 ), pIndexName, TRUE, TRUE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // case 1:
   rc = cl.getIndexes( cursor, "$id" ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "case 1: \n" ) ;
   displayRecord( cursor ) ;

   // case 2:
   rc = cl.getIndexes( cursor, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "case 2: \n" ) ;
   displayRecord( cursor ) ;

   // case 3:
   rc = cl.getIndex( pIndexName, obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "case 3: \n" ) ;
   cout << obj.toString() << endl ;

   // case 4:
   rc = cl.getIndexes( vec ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   printf( "case 4: \n" ) ;
   for (vector<BSONObj>::iterator it = vec.begin();
        it != vec.end(); ++it )
   {
      cout << it->toString() << endl ;
   }

   // case 5:
   rc = cl.getIndex( NULL, obj ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = cl.getIndex( "", obj ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = cl.getIndex( "abc", obj ) ;
   ASSERT_EQ( SDB_IXM_NOTEXIST, rc ) ;
   // disconnect the connection
   connection.disconnect() ;
}

