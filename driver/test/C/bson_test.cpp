#include <iostream>
#include <gtest/gtest.h>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

#include <testcommon.h>
#include <client.h>

using namespace std ;

class bsonTest : public testing::Test
{
   public:
      bsonTest():_db(0),_cl(0)
      {
         _pCLFullName = "bson_test.bson_test" ;
      }
      virtual ~bsonTest() {}

   public:
      static void SetUpTestCase() ;
      static void TearDownTestCase() ;
      virtual void SetUp() ;
      virtual void TearDown() ;

   protected:
      sdbConnectionHandle _db ;
      sdbCollectionHandle _cl ;
      const CHAR *_pCLFullName ;
} ;

void bsonTest::SetUpTestCase()
{
}

void bsonTest::TearDownTestCase()
{
}

void bsonTest::SetUp()
{
   INT32 rc = SDB_OK ; 
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &_db ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson obj ;
   bson_init( &obj ) ;
   rc = bson_append_string( &obj, "PreferedInstance", "M" ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = bson_finish( &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = sdbSetSessionAttr( _db, &obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &obj ) ;

   rc = getCollection ( _db, _pCLFullName, &_cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = sdbTruncateCollection( _db, _pCLFullName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

void bsonTest::TearDown()
{
   sdbDisconnect( _db ) ;
   sdbReleaseCollection( _cl ) ;
   sdbReleaseConnection( _db ) ;
}

void bulk_insert( INT32 totalNum, const CHAR *pCLFullName )
{
   INT32 rc = SDB_OK ;
   #define bulk_num 1000 
   INT32 had_insert_num = 0 ;
   sdbConnectionHandle db = 0 ;
   sdbCollectionHandle cl = 0 ;
   bson *objList[ bulk_num ] = { 0 } ;
  
   rc = sdbConnect ( HOST, SERVER, USER, PASSWD, &db ) ;
   if ( SDB_OK != rc )
   {
      CHECK_MSG( "%s%d\n","failed to connect to database, rc = ",rc ) ;
      goto error ;
   }

   rc = sdbGetCollection( db, pCLFullName, &cl ) ;
   if ( SDB_OK != rc )
   {
      CHECK_MSG( "%s%d\n","failed to get cl handle, rc = ",rc ) ;
      goto error ;
   }

   while( had_insert_num < totalNum )
   {
      INT32 i = 0 ;
      INT32 count = 0 ; 
      INT32 num = totalNum - had_insert_num ;
      num = num > bulk_num ? bulk_num : num ;
      for ( count = 0; count < num; count++ )
      {
         objList[count] = bson_create() ;
         rc = bson_append_int( objList[count], "a", 1 ) ;
         if ( SDB_OK != rc ) 
         {
            CHECK_MSG( "%s%d\n","failed to append int, rc = ",rc ) ; 
            for ( i = 0; i <= count; i++ )
            {
               bson_dispose( objList[i] ) ;
            }
            goto error ;
         }
         rc = bson_finish( objList[count] ) ;
         if ( SDB_OK != rc )
         {
            CHECK_MSG( "%s%d\n","failed to append int, rc = ",rc ) ;
            for ( i = 0; i <= count; i++ )
            {
               bson_dispose( objList[i] ) ;
            }
            goto error ;
         }
      }
      rc = sdbBulkInsert( cl, 0, objList, num ) ;
      if ( SDB_OK != rc )
      {
         CHECK_MSG( "%s%d\n","failed to bulk insert, rc = ",rc ) ;
         for ( i = 0; i < count; i++ )
         {
            bson_dispose( objList[i] ) ;
         }
         goto error ;
      }
      for ( i = 0; i < count; i++ )
      {
         bson_dispose( objList[i] ) ; 
         objList[i] = 0 ;
      }
      had_insert_num += num ; 
   }

done:
   sdbDisconnect( db ) ;
   sdbReleaseCollection ( cl ) ;
   sdbReleaseConnection ( db ) ;
   return ;
error:
   goto done ; 
}

TEST_F( bsonTest, multi_thread_bulk_insert_to_check_oid )
{
   INT32 rc = SDB_OK ;
   INT32 thread_num = 20 ;
   INT32 total_num = 25000 ;
   SINT64 count = 0 ;
   boost::thread_group threads ;
   for ( INT32 i = 0; i < thread_num; i++ )
   {
      threads.create_thread( boost::bind( &bulk_insert, total_num,
                                          _pCLFullName ) ) ;
   }
   threads.join_all() ;

   rc = sdbGetCount( _cl, NULL, &count ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( total_num * thread_num, count ) ;
}

