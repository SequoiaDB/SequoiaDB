/**************************************************************
 * @Description: opreate cl object after disconnect
 *               seqDB-12741 : opreate cl object after disconnect
 * @Modify     : Suqiang Ling
 *               2017-09-11
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include <vector>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class opCL12741 : public testBase
{
protected:
   const char *pCsName ;
   const char *pClName ;
   sdbCollection cl ;

   void SetUp()
   {
      testBase::SetUp() ;

      pCsName = "cs12741" ;
      pClName = "cl12741" ;
      sdbCollectionSpace cs ;

      INT32 rc = SDB_OK ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = cs.createCollection( pClName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;
      db.disconnect() ;
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = SDB_OK ;
         rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to connect db" ;
         rc = db.dropCollectionSpace( pCsName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
      }
      testBase::TearDown() ;
   }
} ;

TEST_F( opCL12741, opCL )
{
   // test all interfaces of class sdbCollection except getCollectionName(), getCSName(), getFullName(), create(), drop(), pop()
   // in the order of c++ api doc
   
   // count
   INT32 rc = SDB_OK ;
   SINT64 count ;
   rc = cl.getCount( count ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "getCount shouldn't be executed" ;

   // split
   BSONObj splitStartCond = BSON( "a" << 0 ) ;
   BSONObj splitEndCond   = BSON( "a" << 100 ) ;
   rc = cl.split( "group1", "group2", splitStartCond, splitEndCond ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "split(1) shouldn't be executed" ;
   FLOAT64 percent = 0.5 ;
   rc = cl.split( "group1", "group2", percent ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "split(2) shouldn't be executed" ;
   SINT64 taskID ;
   rc = cl.splitAsync( taskID, "group1", "group2", splitStartCond, splitEndCond ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "splitAsync(1) shouldn't be executed" ;
   rc = cl.splitAsync( "group1", "group2", percent, taskID ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "splitAsync(2) shouldn't be executed" ;

   // alter cl
   BSONObj alterOpt = BSON( "ShardingKey" << BSON( "a" << 1 ) << "ShardingType" << "hash" ) ;
   rc = cl.alterCollection( alterOpt ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "alter cl shouldn't be executed" ;

   // insert/update/delete
	vector<BSONObj> docs ;
	for( INT32 i = 0; i < 10; ++i )
	{
		BSONObj doc = BSON( "a" << i ) ;
		docs.push_back( doc ) ;
	}
	rc = cl.bulkInsert( FLG_INSERT_CONTONDUP, docs ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "bulkInsert shouldn't be executed" ;
	BSONObj doc = BSON( "a" << 1 ) ;
   rc = cl.insert( doc ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "insert shouldn't be executed" ;
	BSONObj rule = BSON( "$set" << BSON( "a" << 10 ) ) ;
   rc = cl.update( rule ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "update shouldn't be executed" ;
   rc = cl.upsert( rule ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "upsert shouldn't be executed" ;
   rc = cl.del() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "delete shouldn't be executed" ;

   // query
   sdbCursor cursor ;
   rc = cl.query( cursor ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "query shouldn't be executed" ;
   BSONObj obj ;
   rc = cl.queryOne( obj ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "queryOne shouldn't be executed" ;
   rc = cl.queryAndUpdate( cursor, rule ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "queryAndUpdate shouldn't be executed" ;
   rc = cl.queryAndRemove( cursor ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "queryAndRemove shouldn't be executed" ;

   // index 
   BSONObj indexDef = BSON( "a" << 1 ) ;
   rc = cl.createIndex( indexDef, "aIndex", false, false );
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "createIndex(1) shouldn't be executed" ;
   rc = cl.createIndex( indexDef, "aIndex", false, false, 32 ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "createIndex(2) shouldn't be executed" ;
   rc = cl.getIndexes( cursor, "aIndex" ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get indexes shouldn't be executed" ;
   rc = cl.dropIndex( "aIndex" ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "drop index shouldn't be executed" ;

   // other
   vector<BSONObj> objVec ;
   rc = cl.aggregate( cursor, objVec ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "aggregate shouldn't be executed" ;
   BSONObj cond ;
   BSONObj orderBy ;
   BSONObj hint ;
   rc = cl.getQueryMeta( cursor, cond, orderBy, hint, 0, -1 ) ; 
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "getQueryMeta shouldn't be executed" ;

   // attach/detach
   BSONObj attachOpt = BSON( "LowBound" << BSON( "a" << 0 ) 
                          << "UpBound"  << BSON( "a" << 100 ) ) ;
   rc = cl.attachCollection( "foo.bar", attachOpt ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "attach cl shouldn't be executed" ;
   rc = cl.detachCollection( "foo.bar" ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "detach cl shouldn't be executed" ;

   // explain
   rc = cl.explain( cursor ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "explain shouldn't be executed" ;

   // lob
   sdbLob lob ;
   rc = cl.createLob( lob ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "create lob shouldn't be executed" ;
   OID oid ;
   rc = cl.removeLob( oid ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "remove lob shouldn't be executed" ;
   rc = cl.openLob( lob, oid ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "open lob shouldn't be executed" ;
   rc = cl.listLobs( cursor ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "list lobs shouldn't be executed" ;
   rc = cl.listLobPieces( cursor ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "listLobPieces shouldn't be executed" ;

   // truncate
   rc = cl.truncate() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "truncate shouldn't be executed" ;

   // id index
   rc = cl.createIdIndex() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "create id index shouldn't be executed" ;
   rc = cl.dropIdIndex() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "drop id index shouldn't be executed" ;
}
