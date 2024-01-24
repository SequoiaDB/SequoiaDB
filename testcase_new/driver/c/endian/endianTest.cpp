/****************************************************************************
 * @Description: test case for c driver  
 *				     ( manual test case,not in CI )
 *				     test send and recv msg between big-endian and little-endian
 * @Modify:		  Liang xuewang Init
 *				     2016-11-10
 *****************************************************************************/
#include <gtest/gtest.h>
#include <client.h>

const CHAR* hostName = "192.168.28.2" ;  // ppc--big endian  x86--little endian
const CHAR* svcName = "11810" ;

TEST( endianTest, bigAndLittle )
{
   sdbConnectionHandle db = SDB_INVALID_HANDLE ;
   sdbCSHandle cs = SDB_INVALID_HANDLE ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   const CHAR* csName = "endianTestCs" ;
   const CHAR* clName = "endianTestCl" ;
   INT32 rc = SDB_OK ;

   rc = sdbConnect( hostName, svcName, "", "", &db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
   rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   rc = sdbCreateCollection( cs, clName, &cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

   // insert record { a: 1 }
   bson obj ;
   bson_init( &obj ) ;
   bson_append_int( &obj, "a", 1 ) ;
   bson_finish( &obj ) ;
   rc = sdbInsert( cl, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert record { a: 1 }" ;

   // query record { a: 1 }
   bson sel ;
   bson_init( &sel ) ;
   bson_append_string( &sel, "a", "" ) ;
   bson_finish( &sel ) ;
   rc = sdbQuery( cl, &obj, &sel, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query record { a: 1 }" ;

   // check result
   bson ret ;
   bson_init( &ret ) ;
   rc = sdbNext( cursor, &ret ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get result" ;
   bson_iterator it ;
   bson_iterator_init( &it, &ret ) ;
   ASSERT_EQ( 1, bson_iterator_int(&it) ) << "fail to check result" ;
   sdbReleaseCursor( cursor ) ;

   // update record { $set: { a: 100 } }
   bson update ;
   bson_init( &update ) ;
   bson_append_start_object( &update, "$set" ) ;
   bson_append_int( &update, "a", 100 ) ;
   bson_append_finish_object( &update ) ;
   bson_finish( &update ) ;
   rc = sdbUpdate( cl, &update, &obj, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to update record" ;

   // query record { a: 100 }
   bson cond ;
   bson_init( &cond ) ;
   bson_append_int( &cond, "a", 100 ) ;
   bson_finish( &cond ) ;
   rc = sdbQuery( cl, &cond, &sel, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query record { a: 100 }" ;

   // check result
   bson_destroy( &ret ) ;
   bson_init( &ret ) ;
   rc = sdbNext( cursor, &ret ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get result" ;
   bson_iterator_init( &it, &ret ) ;
   ASSERT_EQ( 100, bson_iterator_int(&it) ) << "fail to check result" ;
   sdbReleaseCursor( cursor ) ;

   // delete record { a: 100 }
   rc = sdbDelete( cl, &cond, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to delete record { a: 100 }" ;

   // query record { a: 100 }
   rc = sdbQuery( cl, &cond, &sel, NULL, NULL, 0, -1, &cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query record { a: 100 }" ;

   // check result
   bson_destroy( &ret ) ;
   bson_init( &ret ) ;
   rc = sdbNext( cursor, &ret ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to check delete" ;

   // destroy bson
   bson_destroy( &obj ) ;
   bson_destroy( &sel ) ;
   bson_destroy( &ret ) ;
   bson_destroy( &update ) ;
   bson_destroy( &cond ) ;

   // drop cs
   rc = sdbDropCollectionSpace( db, csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;

   // release handle
   sdbDisconnect( db ) ;
   sdbReleaseCursor( cursor ) ;
   sdbReleaseCollection( cl ) ;
   sdbReleaseCS( cs ) ;
   sdbReleaseConnection( db ) ;
}
