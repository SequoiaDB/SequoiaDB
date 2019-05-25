/*****************************************************************************
*@Description : SequoiaDB CPP driver testcase : Query()
*@Modify List :
*               2014-10-27   xiaojun  Hu   Init
*****************************************************************************/
#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;
/*****************************************************************************
*@Description : Query one record, specify argument : numToReturn = 0 [limit].
*@Modify List :
*               2014-10-27   xiaojun  Hu   Init
*****************************************************************************/
TEST( colleciton, sdbCppQueryOne )
{
   INT32 rc = SDB_OK ;
   const CHAR *csName = "sdb_clientcpp_collection_test" ;
   const CHAR *clName = "sdb_query_limit_one" ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor ;

   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.dropCollectionSpace( csName ) ;
   rc = db.createCollectionSpace( csName, SDB_PAGESIZE_DEFAULT, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj obj = BSON( "ReplSize" << 0 ) ;   //replsize = 0
   rc = cs.createCollection( clName, obj, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   for ( INT32 i = 0; i < 200; i++ )
   {
      BSONObj obj = BSON ( "firstName" << "John" <<
                           "lastName" << "Smith" << "age" << i ) ;
      BSONObj cond = BSON( "age" << BSON ( "$lt" << 20) ) ;
      rc = cl.insert( obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   /*****************************************************************
   * specify query condition is none[TestPoint_1]
   *****************************************************************/
   BSONObj cond ;
   BSONObj select ;
   BSONObj orderBy ;
   BSONObj hint ;
   rc = cl.query( cursor, cond, select, orderBy, hint, 0, 1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   INT32 count = 0 ;
   BSONObj record ;
   while ( SDB_OK == ( rc = cursor.next( record ) ) )
   {
      cout << record.toString() <<endl ;
      ++count ;
   }
   cout << "specify none condition, count : " << count << endl ;
   ASSERT_EQ( 1, count ) ;
   /*****************************************************************
   * specify query condition range[TestPoint_2]
   *****************************************************************/
   cond = BSON( "age" << BSON( "$lt" << 20 ) ) ;
   rc = cl.query( cursor, cond, select, orderBy, hint, 0, 1 ) ;
   ASSERT_EQ( SDB_OK, rc  ) ;
   count = 0 ;
   while ( 0 == ( rc = cursor.next( record ) ) )
   {
      cout << record.toString() <<endl ;
      count++ ;
   }
   cout << "specify range condition, count : " << count << endl ;
   ASSERT_EQ( 1, count ) ;
   /*****************************************************************
   * specify query specify one[TestPoint_3]
   *****************************************************************/
   cond = BSON( "age" << 50 ) ;
   rc = cl.query( cursor, cond, select, orderBy, hint, 0, 1 ) ;
   ASSERT_EQ( SDB_OK, rc  ) ;
   count = 0 ;
   while ( 0 == (rc = cursor.next(record)) )
   {
      cout << record.toString() <<endl ;
      count++ ;
   }
   cout << "specify one record, count : " << count << endl ;
   ASSERT_EQ( 1, count ) ;
   /*****************************************************************
   * specify query same field int record[TestPoint_4]
   *****************************************************************/
   cond = BSON( "firstName" << "John" ) ;
   rc = cl.query( cursor, cond, select, orderBy, hint, 0, 1 ) ;
   ASSERT_EQ( SDB_OK, rc  ) ;
   count = 0 ;
   while ( 0 == ( rc = cursor.next(record) ) )
   {
      cout << record.toString() <<endl ;
      count++ ;
   }
   cout << "specify same field in record, count : " << count << endl ;
   ASSERT_EQ( 1, count ) ;
   db.disconnect() ;
}
