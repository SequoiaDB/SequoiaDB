/**************************************************************
 * @Description: reset snapshot
 *               seqDB-12664 : reset snapshot
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

class resetSnapshot14455 : public testBase 
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;
   CHAR rgName[128] ;

   void SetUp() 
   {
      testBase::SetUp() ;
      if( isStandalone( db ) ) 
      {   
         cout << "Run mode is standalone" << endl ;
         return ; 
      }   

      INT32 rc = SDB_OK ;
      pCsName = "resetSnapshot14455" ;
      pClName = "resetSnapshot14455" ;
      sdbCollectionSpace cs ;
      sdbCollection cl ;
      BSONObj option ;
      vector< string > groups ;
      INT32 i = 0 ;
      const INT32 docNum = 100 ;
      vector< BSONObj > docs ;

      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = getGroups( db, groups ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get groups" ;
      strcpy( rgName, groups[0].c_str() ) ;
      option = BSON( "Group" << rgName ) ;
      rc = cs.createCollection( pClName, option, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;

      for( i = 0 ; i < docNum ; ++i )
      {
         docs.push_back( BSON( "a" << 1 ) ) ;
      }
      rc = cl.bulkInsert( 0, docs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   }

   void TearDown() 
   {
      INT32 rc = SDB_OK ;
      if( isStandalone( db ) ) 
      {   
         cout << "Run mode is standalone" << endl ;
      } 
      else if( shouldClear() )
      {
         rc = db.dropCollectionSpace( pCsName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
      }
      testBase::TearDown() ;
   }

   INT32 connectNodeMaster( const CHAR *pRgName, sdb& dataDB )
   {
      INT32 rc = SDB_OK ;
      sdbReplicaGroup group ;
      sdbNode node ;

      rc = db.getReplicaGroup( pRgName, group ) ;
      CHECK_RC( SDB_OK, rc, "fail to get group by name" ) ;
      rc = group.getMaster( node ) ;
      CHECK_RC( SDB_OK, rc, "fail to get master node" ) ;
      rc = node.connect( dataDB ) ;
      CHECK_RC( SDB_OK, rc, "fail to connect dataDB" ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 createStatisInfo( sdb &dataDB )
   {
      INT32 rc = SDB_OK ;
      sdbCollectionSpace cs ;
      sdbCollection cl ;
      sdbCursor cur ;
      BSONObj obj ;

      rc = dataDB.getCollectionSpace( pCsName, cs ) ;
      CHECK_RC( SDB_OK, rc, "fail to get cs" ) ;
      rc = cs.getCollection( pClName, cl ) ;
      CHECK_RC( SDB_OK, rc, "fail to get cl" ) ;
      rc = cl.query( cur ) ;
      CHECK_RC( SDB_OK, rc, "fail to query" ) ;
      
      while( !( rc = cur.next( obj ) ) ) {}
      CHECK_RC( SDB_DMS_EOC, rc, "fail to get next" ) ;
      rc = SDB_OK ;

   done:
      cur.close() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 isDataBaseSnapClean( sdb &dataDB, BOOLEAN *isClean ) {
      INT32 rc = SDB_OK ;
      sdbCursor cur ;
      BSONObj obj ;
      INT64 totalRead ;

      rc = dataDB.getSnapshot( cur, SDB_SNAP_DATABASE ) ;
      CHECK_RC( SDB_OK, rc, "fail to get snapshot" ) ;
      rc = cur.next( obj ) ;
      CHECK_RC( SDB_OK, rc, "fail to get next" ) ;
      totalRead = obj.getField( "TotalRead" ).Long() ;
      *isClean = ( totalRead == 0 ) ;

   done:
      cur.close() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 isSessionSnapClean( sdb &dataDB, BOOLEAN *isClean ) {
      INT32 rc = SDB_OK ;
      sdbCursor cur ;
      BSONObj obj ;
      INT64 totalRead ;

      rc = dataDB.getSnapshot( cur, SDB_SNAP_SESSIONS, BSON( "Type" << "Agent" ) ) ;
      CHECK_RC( SDB_OK, rc, "fail to get snapshot" ) ;
      rc = cur.next( obj ) ;
      CHECK_RC( SDB_OK, rc, "fail to get next" ) ;
      totalRead = obj.getField( "TotalRead" ).Long() ;
      *isClean = ( totalRead == 0 ) ;

   done:
      cur.close() ;
      return rc ;
   error:
      goto done ;
   }
} ;

TEST_F( resetSnapshot14455, resetSnapshot )
{
   if( isStandalone( db ) ) 
   {   
      cout << "Run mode is standalone" << endl ;
      return ; 
   }   
   
   INT32 rc = SDB_OK ;
   sdb dataDB ;
   BOOLEAN isClean ;

   rc = connectNodeMaster( rgName, dataDB ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect node" ;
   rc = createStatisInfo( dataDB ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create statistics info" ;

   rc = db.resetSnapshot( BSON( "Type" << "sessions" ) );
   ASSERT_EQ( SDB_OK, rc ) << "fail to reset snapshot" ;

   rc = isSessionSnapClean( dataDB, &isClean ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to check session snapshot" ;
   ASSERT_TRUE( isClean ) << "snapshot-session has not been reset" ;
   rc = isDataBaseSnapClean( dataDB, &isClean ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to check database snapshot" ;
   ASSERT_FALSE( isClean ) << "snapshot-database shouldn't be reset" ;

   dataDB.disconnect() ;
}
