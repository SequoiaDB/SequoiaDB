#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"

using namespace sdbclient ;

const CHAR *MAIN_CS_NAME = "maincs" ;
const CHAR *MAIN_CL_NAME = "maincl" ;
const CHAR *SUB_CS_NAME  = "cs" ;
const CHAR *SUB_CL_NAME_A = "subclA" ;
const CHAR *SUB_CL_NAME_B = "subclB" ;
const CHAR *SUB_CL_NAME_A_FULL = "cs.subclA" ;
const CHAR *SUB_CL_NAME_B_FULL = "cs.subclB" ;
const CHAR *SPLIT_CS_NAME = "splitcs" ;

sdb connection ;

class InsertUpdateOnDup : public testing::Test
{
protected:
   static void SetUpTestCase()
   {
      INT32 rc = SDB_OK ;
      sdbCollectionSpace cs ;
      sdbCollection cl ;

      rc = initEnv() ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = connection.connect( HOST, SERVER, USER, PASSWD ) ;
      ASSERT_EQ( SDB_OK, rc ) ;

      // Clear environment
      connection.dropCollectionSpace( MAIN_CS_NAME ) ;
      connection.dropCollectionSpace( SUB_CS_NAME ) ;

      BSONObj indexDef = BSON( "a" << 1 ) ;
      rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = getCollection( cs, COLLECTION_NAME, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = cl.createIndex( indexDef, "idx", TRUE, FALSE ) ;
      ASSERT_EQ( SDB_OK, rc ) ;

      _prepareMainSubCL() ;
      _prepareHashShardCL();
      _prepareRangeShardCL();
   }

   static void TearDownTestCase()
   {
      connection.dropCollectionSpace( MAIN_CS_NAME ) ;
      connection.dropCollectionSpace( SUB_CS_NAME ) ;
      connection.dropCollectionSpace( SPLIT_CS_NAME ) ;
      connection.dropDomain( "dupdomain" ) ;
   }

   virtual void SetUp()
   {
      INT32 rc = SDB_OK ;
      sdbCollectionSpace cs ;
      sdbCollection cl ;

      rc = getCollectionSpace( connection, COLLECTION_SPACE_NAME, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = getCollection( cs, COLLECTION_NAME, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = cl.truncate() ;
      ASSERT_EQ( SDB_OK, rc ) ;

      rc = getCollectionSpace( connection, MAIN_CS_NAME, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = getCollection( cs, MAIN_CL_NAME, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      rc = cl.truncate() ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   virtual void TearDown()
   {
   }

   static void _prepareHashShardCL() ;
   static void _prepareRangeShardCL() ;
   static void _prepareMainSubCL() ;

protected:
} ;

void InsertUpdateOnDup::_prepareHashShardCL()
{
   INT32 rc = SDB_OK ;
   sdbDomain dupdomain ;
   sdbCollectionSpace splitCS ;
   sdbCollection hashCL ;


   BSONObj domainOption = BSON( "Groups" << BSON_ARRAY( "db1" << "db2" )
                                         << "AutoSplit" << true ) ;
   //cout<<domainOption.toString(0,1,1)<<endl;
   rc = connection.createDomain( "dupdomain", domainOption, dupdomain ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObj splitCSOption = BSON( "Domain" << "dupdomain" ) ;
   rc = connection.createCollectionSpace( SPLIT_CS_NAME, splitCSOption, splitCS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObj hashCLOption = BSON( "AutoSplit" << true <<
                                            "ShardingKey" << BSON( "a" << 1 ) <<
                                            "ShardingType" << "hash" ) ;
   rc = splitCS.createCollection( "hashcl", hashCLOption, hashCL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObj indexDef = BSON( "a" << 1 ) ;
   rc = hashCL.createIndex( indexDef, "idx", TRUE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

void InsertUpdateOnDup::_prepareRangeShardCL()
{
   INT32 rc = SDB_OK ;
   sdbDomain dupdomain ;
   sdbCollectionSpace splitCS ;
   sdbCollection rangeCL ;

   rc = connection.getCollectionSpace( SPLIT_CS_NAME, splitCS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObj rangeCLOption = BSON( "ShardingKey" << BSON( "a" << 1 ) <<
                                               "ShardingType" << "range" << "Group" << "db1") ;
   rc = splitCS.createCollection( "rangecl", rangeCLOption, rangeCL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObj indexDef = BSON( "a" << 1 ) ;
   rc = rangeCL.createIndex( indexDef, "idx", TRUE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

void InsertUpdateOnDup::_prepareMainSubCL()
{
   INT32 rc = SDB_OK ;
   sdbCollectionSpace mainCS ;
   sdbCollectionSpace subCS ;
   sdbCollection mainCL ;
   sdbCollection subCLA, subCLB ;

   rc = connection.createCollectionSpace( MAIN_CS_NAME, 65536, mainCS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObj mainCLOption = BSON( "IsMainCL" << true <<
         "ShardingKey" << BSON( "a" << 1 ) <<
         "ShardingType" << "range" ) ;

   rc = mainCS.createCollection( MAIN_CL_NAME, mainCLOption, mainCL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = connection.createCollectionSpace( SUB_CS_NAME, 65536, subCS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = subCS.createCollection( SUB_CL_NAME_A, subCLA ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = subCS.createCollection( SUB_CL_NAME_B, subCLB ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObj rangeA = BSON( "LowBound" << BSON( "a" << 0 ) << "UpBound" << BSON( "a" << 100) ) ;
   BSONObj rangeB = BSON( "LowBound" << BSON( "a" << 100 ) << "UpBound" << BSON( "a" << 200 ) ) ;

   rc = mainCL.attachCollection( SUB_CL_NAME_A_FULL, rangeA ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = mainCL.attachCollection( SUB_CL_NAME_B_FULL, rangeB ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObj indexDef = BSON( "a" << 1 ) ;
   rc = mainCL.createIndex( indexDef, "idx", TRUE, FALSE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

static BSONObj buildInsertHint( const BSONObj &updator )
{
   BSONObjBuilder builder ;
   BSONObjBuilder modifyBuilder( builder.subobjStart( "$Modify" ) ) ;
   modifyBuilder.append( "OP", "update" ) ;
   BSONObjBuilder updateBuilder( modifyBuilder.subobjStart( "Update" ) ) ;
   updateBuilder.appendElements( updator ) ;
   updateBuilder.done() ;
   modifyBuilder.done() ;

   return builder.obj() ;
}

static void checkInsertResult( const BSONObj &result, UINT64 insertedNum,
                               UINT64 duplicatedNum, UINT64 modifiedNum )
{
   const CHAR *INSERT_STR = "InsertedNum" ;
   const CHAR *DUPLICATE_STR = "DuplicatedNum" ;
   const CHAR *MODIFY_STR = "ModifiedNum" ;
   ASSERT_EQ( insertedNum, result.getField( INSERT_STR ).numberLong() ) ;
   ASSERT_EQ( duplicatedNum, result.getField( DUPLICATE_STR ).numberLong() ) ;
   ASSERT_EQ( modifiedNum, result.getField( MODIFY_STR ).numberLong() ) ;
}

TEST_F(InsertUpdateOnDup, insert_update_on_dup_simple)
{
   INT32 rc = SDB_OK ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   const CHAR *pHostName = HOST ;
   const CHAR *pPort     = SERVER ;
   const CHAR *pUsr      = USER ;
   const CHAR *pPasswd   = PASSWD ;
   BSONObj indexDef = BSON( "a" << 1 ) ;

   rc = getCollectionSpace( connection, MAIN_CS_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection( cs, MAIN_CL_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObj record = BSON( "a" << 1 << "b" << 2 ) ;
   rc = cl.insert(record) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson::BSONObj realRecord ;
   rc = cl.queryOne( realRecord ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson::BSONElement idEle = realRecord.getField( "_id" ) ;

   BSONObj updator = BSON( "$set" << BSON( "b" << 100 ) )  ;
   BSONObj hint = buildInsertHint( updator ) ;
   BSONObj result ;

   rc = cl.insert( record, hint, FLG_INSERT_UPDATEONDUP, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   checkInsertResult( result, 0, 1, 1 ) ;
   INT32 count = 0 ;
   bson::BSONObj expectRecord ;
   bson::BSONObjBuilder builder ;
   builder.append( idEle ) ;
   builder.append( "a", 1 ) ;
   builder.append( "b", 100 ) ;
   expectRecord = builder.done() ;

   sdbCursor cursor ;
   rc = cl.query( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   while ( SDB_OK == ( rc = cursor.next( realRecord ) ) )
   {
      ASSERT_EQ( 0, realRecord.woCompare( expectRecord ) ) ;
      ++count ;
   }

   ASSERT_EQ( 1, count ) ;
}

TEST_F(InsertUpdateOnDup, insert_update_on_dup_update_shardkey_err)
{
   INT32 rc = SDB_OK ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   const CHAR *pHostName = HOST ;
   const CHAR *pPort     = SERVER ;
   const CHAR *pUsr      = USER ;
   const CHAR *pPasswd   = PASSWD ;
   BSONObj indexDef = BSON( "a" << 1 ) ;

   rc = getCollectionSpace( connection, MAIN_CS_NAME, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = getCollection( cs, MAIN_CL_NAME, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObj record = BSON( "a" << 1 << "b" << 2 ) ;
   rc = cl.insert(record) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson::BSONObj realRecord ;
   rc = cl.queryOne( realRecord ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   bson::BSONElement idEle = realRecord.getField( "_id" ) ;

   BSONObj updator = BSON( "$set" << BSON( "a" << 100 ) )  ;
   BSONObj hint = buildInsertHint( updator ) ;

   rc = cl.insert( record, hint, FLG_INSERT_UPDATEONDUP ) ;
   ASSERT_EQ( SDB_UPDATE_SHARD_KEY, rc ) ;
   sdbCursor cursor ;
   rc = cl.query( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   INT32 count = 0 ;
   bson::BSONObj expectRecord ;
   bson::BSONObjBuilder builder ;
   builder.append( idEle ) ;
   builder.append( "a", 1 ) ;
   builder.append( "b", 2 ) ;
   expectRecord = builder.done() ;

   while ( SDB_OK == ( rc = cursor.next( realRecord ) ) )
   {
      ASSERT_EQ( 0, realRecord.woCompare( expectRecord ) ) ;
      ++count ;
   }

   ASSERT_EQ( 1, count ) ;
}

TEST_F(InsertUpdateOnDup, insert_update_on_dup_main_insert)
{
   INT32 rc = SDB_OK ;
   sdbCollectionSpace maincs ;
   sdbCollection maincl ;

   rc = getCollectionSpace( connection, MAIN_CS_NAME, maincs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = getCollection( maincs, MAIN_CL_NAME, maincl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // Insert 200 records into the main collection, which has two sub
   // collections.
   for ( INT32 i = 0; i < 200; ++i )
   {
      BSONObj record = BSON( "_id"<< i << "a" << i << "b" << i ) ;
      rc = maincl.insert( record ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   BSONObj record1 = BSON( "a" << 10 << "b" << 1000 ) ;
   BSONObj record2 = BSON( "a" << 110 << "b" << 1100 ) ;
   BSONObj updator = BSON( "$inc"<< BSON( "b" << 1 ) ) ;
   BSONObj hint = buildInsertHint( updator ) ;
   BSONObj result ;

   rc = maincl.insert( record1, hint, FLG_INSERT_UPDATEONDUP, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   checkInsertResult( result, 0, 1, 1 ) ;
   rc = maincl.insert( record2, hint, FLG_INSERT_UPDATEONDUP, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   checkInsertResult( result, 0, 1, 1 ) ;

   sdbCursor cursor ;
   BSONObj realRecord ;
   vector< BSONObj > conditions ;
   vector< BSONObj > expectRecords ;

   conditions.push_back( BSON( "a" << 10 ) ) ;
   conditions.push_back( BSON( "a" << 110 ) ) ;
   expectRecords.push_back( BSON( "_id" << 10 << "a" << 10 << "b" << 11 ) ) ;
   expectRecords.push_back( BSON( "_id" << 110 << "a" << 110 << "b" << 111 ) ) ;

   vector< BSONObj >::iterator condItr = conditions.begin()  ;
   vector< BSONObj >::iterator expectRecordItr = expectRecords.begin() ;

   for ( ; condItr != conditions.end(); ++condItr, ++expectRecordItr )
   {
      rc = maincl.query( cursor, *condItr ) ;
      ASSERT_EQ( SDB_OK, rc ) ;

      rc = cursor.next( realRecord ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      ASSERT_EQ( 0, realRecord.woCompare( *expectRecordItr ) ) ;

      rc = cursor.next( realRecord ) ;
      ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   }
}

TEST_F(InsertUpdateOnDup, insert_update_on_dup_main_bulk_insert)
{
   INT32 rc = SDB_OK ;
   sdbCollectionSpace maincs ;
   sdbCollection maincl ;

   rc = getCollectionSpace( connection, MAIN_CS_NAME, maincs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = getCollection( maincs, MAIN_CL_NAME, maincl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // Insert 200 records into the main collection, which has two sub
   // collections.
   for ( INT32 i = 0; i < 200; ++i )
   {
      BSONObj record = BSON( "_id"<< i << "a" << i << "b" << i ) ;
      rc = maincl.insert( record ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   vector< BSONObj > records ;
   records.push_back( BSON( "a" << 10 << "b" << 1000 ) ) ;
   records.push_back( BSON( "a" << 110 << "b" << 1100 ) ) ;
   BSONObj updator = BSON( "$inc"<< BSON( "b" << 1 ) ) ;
   BSONObj hint = buildInsertHint( updator ) ;
   BSONObj result ;

   rc = maincl.insert( records, hint, FLG_INSERT_UPDATEONDUP, &result ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   checkInsertResult( result, 0, 2, 2 ) ;

   sdbCursor cursor ;
   BSONObj realRecord ;
   vector< BSONObj > conditions ;
   vector< BSONObj > expectRecords ;

   conditions.push_back( BSON( "a" << 10 ) ) ;
   conditions.push_back( BSON( "a" << 110 ) ) ;
   expectRecords.push_back( BSON( "_id" << 10 << "a" << 10 << "b" << 11 ) ) ;
   expectRecords.push_back( BSON( "_id" << 110 << "a" << 110 << "b" << 111 ) ) ;

   vector< BSONObj >::iterator condItr = conditions.begin()  ;
   vector< BSONObj >::iterator expectRecordItr = expectRecords.begin() ;

   for ( ; condItr != conditions.end(); ++condItr, ++expectRecordItr )
   {
      rc = maincl.query( cursor, *condItr ) ;
      ASSERT_EQ( SDB_OK, rc ) ;

      rc = cursor.next( realRecord ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      ASSERT_EQ( 0, realRecord.woCompare( *expectRecordItr ) ) ;

      rc = cursor.next( realRecord ) ;
      ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   }
}

TEST_F(InsertUpdateOnDup, insert_update_on_dup_hashsplit_insert)
{
   INT32 rc = SDB_OK ;
   sdbCollectionSpace splitCS ;
   sdbCollection hashcl ;

   rc = getCollectionSpace( connection, SPLIT_CS_NAME, splitCS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = getCollection( splitCS, "hashcl", hashcl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // Insert 200 records into the main collection, which split into two groups.
   for ( INT32 i = 0; i < 200; ++i )
   {
      BSONObj record = BSON( "_id"<< i << "a" << i << "b" << i ) ;
      rc = hashcl.insert( record ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   BSONObj updator = BSON( "$inc"<< BSON( "b" << 1 ) ) ;
   BSONObj hint = buildInsertHint( updator ) ;
   for ( INT32 i = 90; i < 110; ++i )
   {
      BSONObj result ;
      BSONObj record = BSON( "_id"<< i << "a" << i << "b" << i ) ;
      rc = hashcl.insert( record, hint, FLG_INSERT_UPDATEONDUP, &result ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      checkInsertResult( result, 0, 1, 1 ) ;
   }


   sdbCursor cursor ;
   BSONObj realRecord ;
   vector< BSONObj > conditions ;
   vector< BSONObj > expectRecords ;

   for ( INT32 i = 90; i < 110; ++i )
   {
      conditions.push_back( BSON( "a" << i ) ) ;
      expectRecords.push_back( BSON( "_id" << i << "a" << i << "b" << i+1 ) ) ;
   }

   vector< BSONObj >::iterator condItr = conditions.begin()  ;
   vector< BSONObj >::iterator expectRecordItr = expectRecords.begin() ;

   for ( ; condItr != conditions.end(); ++condItr, ++expectRecordItr )
   {
      rc = hashcl.query( cursor, *condItr ) ;
      ASSERT_EQ( SDB_OK, rc ) ;

      rc = cursor.next( realRecord ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      ASSERT_EQ( 0, realRecord.woCompare( *expectRecordItr ) ) ;

      rc = cursor.next( realRecord ) ;
      ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   }
}

TEST_F(InsertUpdateOnDup, insert_update_on_dup_rangesplit_insert)
{

   INT32 rc = SDB_OK ;
   sdbCollectionSpace splitcs ;
   sdbCollection rangecl ;

   rc = getCollectionSpace( connection, SPLIT_CS_NAME, splitcs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = getCollection( splitcs, "rangecl", rangecl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   for ( INT32 i = 0; i < 200; ++i )
   {
      BSONObj record = BSON( "_id"<< i << "a" << i << "b" << i ) ;
      rc = rangecl.insert( record ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   BSONObj leftbound = BSON( "a"<< 100 ) ;
   rangecl.split("db1","db2",leftbound);

   BSONObj updator = BSON( "$inc"<< BSON( "b" << 1 ) ) ;
   BSONObj hint = buildInsertHint( updator ) ;
   for ( INT32 i = 90; i < 110; ++i )
   {
      BSONObj result ;
      BSONObj record = BSON( "_id"<< i << "a" << i << "b" << i ) ;
      rc = rangecl.insert( record, hint, FLG_INSERT_UPDATEONDUP, &result ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      checkInsertResult( result, 0, 1, 1 ) ;
   }

   sdbCursor cursor ;
   BSONObj realRecord ;
   vector< BSONObj > conditions ;
   vector< BSONObj > expectRecords ;

   for ( INT32 i = 90; i < 110; ++i )
   {
      conditions.push_back( BSON( "a" << i ) ) ;
      expectRecords.push_back( BSON( "_id" << i << "a" << i << "b" << i+1 ) ) ;
   }

   vector< BSONObj >::iterator condItr = conditions.begin()  ;
   vector< BSONObj >::iterator expectRecordItr = expectRecords.begin() ;

   for ( ; condItr != conditions.end(); ++condItr, ++expectRecordItr )
   {
      rc = rangecl.query( cursor, *condItr ) ;
      ASSERT_EQ( SDB_OK, rc ) ;

      rc = cursor.next( realRecord ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      ASSERT_EQ( 0, realRecord.woCompare( *expectRecordItr ) ) ;

      rc = cursor.next( realRecord ) ;
      ASSERT_EQ( SDB_DMS_EOC, rc ) ;
   }
}

