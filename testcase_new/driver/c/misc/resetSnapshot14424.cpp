/**************************************************************
 * @Description: seqDB-14424: 重置快照接口测试
 * @Modify:      Suqiang Ling Init
 *			 	     2018-02-16
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class resetSnapshot14424 : public testBase
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;
   CHAR rgName[128] ;
   sdbCSHandle cs ;
   sdbCollectionHandle cl ;

   void SetUp()
   {
      testBase::SetUp() ;
      if( isStandalone( db ) )
      {
         printf( "Run mode is standalone\n" ) ;
         return ;
      }

      INT32 rc = SDB_OK ;
      pCsName = "resetSnapshot14424" ;
      pClName = "resetSnapshot14424" ;
      vector<string> groups ;
      bson option ;
      bson doc ;
      INT32 docNum = 20 ;

      // create cs cl
      rc = sdbCreateCollectionSpace( db, pCsName, SDB_PAGESIZE_4K, &cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << pCsName ;

      rc = getGroups( db, groups ) ; 
      ASSERT_EQ( SDB_OK, rc ) << "fail to get data groups" ;
      strcpy( rgName, groups[0].c_str() ) ;

      bson_init( &option ) ;
      bson_append_string( &option, "Group", rgName ) ;
      bson_finish( &option ) ;
      rc = sdbCreateCollection1( cs, pClName, &option, &cl ) ;
      bson_destroy( &option ) ;
      
      // insert a few documents
      while ( docNum-- )
      {
         bson_init( &doc ) ;
         bson_append_int( &doc, "a", 1 ) ;
         bson_finish( &doc ) ;
         rc = sdbInsert( cl, &doc ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
         bson_destroy( &doc ) ;
      }
   }

   void TearDown()
   {
      INT32 rc = SDB_OK ;
      if( isStandalone( db ) )
      {
         printf( "Run mode is standalone\n" ) ;
      }
      else if( shouldClear() )
      {
         rc = sdbDropCollectionSpace( db, pCsName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << pCsName ;
         sdbReleaseCS( cs ) ;
         sdbReleaseCollection( cl ) ;
      } 
      testBase::TearDown() ;
   }

   INT32 connectNodeMaster( const CHAR *rgName, sdbConnectionHandle *pDataDB )
   {
      INT32 rc                   = SDB_OK ;
      sdbReplicaGroupHandle rg   = 0 ;
      sdbNodeHandle node         = 0 ;
      const CHAR *pHostName ;
      const CHAR *pSvcName ;
      const CHAR *pNodeName ;
      INT32 nodeID ;

      rc = sdbGetReplicaGroup( db, rgName, &rg ) ;
      CHECK_RC( SDB_OK, rc, "fail to get group by name" ) ;
      rc = sdbGetNodeMaster( rg, &node) ;
      CHECK_RC( SDB_OK, rc, "fail to get node" ) ;
      rc = sdbGetNodeAddr( node, &pHostName, &pSvcName, &pNodeName, &nodeID ) ;
      CHECK_RC( SDB_OK, rc, "fail to get node address" ) ;
      rc = sdbConnect( pHostName, pSvcName, ARGS->user(), ARGS->passwd(), pDataDB ) ;
      CHECK_RC( SDB_OK, rc, "fail to connect sdb" ) ;

   done:
      sdbReleaseNode( node ) ;
      sdbReleaseReplicaGroup( rg ) ;
      return rc ;
   error:
      goto done ;
   }
   
   INT32 createStatisInfo( sdbConnectionHandle dataDB )
   {
      INT32 rc                = SDB_OK ;
      sdbCSHandle cs          = 0 ;
      sdbCollectionHandle cl  = 0 ;
      sdbCursorHandle cur     = 0 ;
      bson obj ;

      rc = sdbGetCollectionSpace( dataDB, pCsName, &cs ) ;
      CHECK_RC( SDB_OK, rc, "fail to get cs" ) ;
      rc = sdbGetCollection1( cs, pClName, &cl ) ;
      CHECK_RC( SDB_OK, rc, "fail to get cl" ) ;
      rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cur ) ;
      CHECK_RC( SDB_OK, rc, "fail to query" ) ;
      
      bson_init( &obj ) ; 
      while( !( rc = sdbNext( cur, &obj ) ) ) {}
      bson_destroy( &obj ) ;
      CHECK_RC( SDB_DMS_EOC, rc, "fail to get next" ) ;
      rc = SDB_OK ;

   done:
      sdbReleaseCursor( cur ) ;
      sdbReleaseCollection( cl ) ;
      sdbReleaseCS( cs ) ;
      return rc ;
   error:
      goto done ;
   }
   
   INT32 isDataBaseSnapClean( sdbConnectionHandle dataDB, BOOLEAN *isClean ) {
      INT32 rc             = SDB_OK ;
      sdbCursorHandle cur  = 0 ;
      bson obj ;
      bson_iterator i ;
      bson_type type ;
      INT64 totalRead ;

      rc = sdbGetSnapshot( dataDB, SDB_SNAP_DATABASE, NULL, NULL, NULL, &cur ) ;
      CHECK_RC( SDB_OK, rc, "fail to get snapshot" ) ;
      bson_init( &obj ) ; 
      rc = sdbNext( cur, &obj ) ;
      CHECK_RC( SDB_OK, rc, "fail to get next" ) ;
      type = bson_find( &i, &obj, "TotalRead" ) ;
      if( BSON_EOO != type )
         totalRead = bson_iterator_long( &i ) ;
      else
         CHECK_RC( SDB_OK, SDB_TEST_ERROR, "no field named 'TotalRead'" ) ;
      bson_destroy( &obj ) ;

      *isClean = ( totalRead == 0 ) ;

   done:
      sdbReleaseCursor( cur ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 isSessionSnapClean( sdbConnectionHandle dataDB, BOOLEAN *isClean ) {
      INT32 rc             = SDB_OK ;
      sdbCursorHandle cur  = 0 ;
      bson cond ;
      bson obj ;
      bson_iterator i ;
      bson_type type ;
      INT64 totalRead ;

      bson_init( &cond ) ;
      bson_append_string( &cond, "Type", "Agent" ) ;
      bson_finish( &cond ) ;
      rc = sdbGetSnapshot( dataDB, SDB_SNAP_SESSIONS, &cond, NULL, NULL, &cur ) ;
      bson_destroy( &cond ) ;
      CHECK_RC( SDB_OK, rc, "fail to get snapshot" ) ;

      bson_init( &obj ) ; 
      rc = sdbNext( cur, &obj ) ;
      CHECK_RC( SDB_OK, rc, "fail to get next" ) ;
      type = bson_find( &i, &obj, "TotalRead" ) ;
      if( BSON_EOO != type )
         totalRead = bson_iterator_long( &i ) ;
      else
         CHECK_RC( SDB_OK, SDB_TEST_ERROR, "no field named 'TotalRead'" ) ;
      bson_destroy( &obj ) ;

      *isClean = ( totalRead == 0 ) ;

   done:
      sdbReleaseCursor( cur ) ;
      return rc ;
   error:
      goto done ;
   }

} ;

TEST_F( resetSnapshot14424, test )
{
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   INT32 rc                   = SDB_OK ;
   sdbConnectionHandle dataDB = 0 ;
   bson option ;
   BOOLEAN isClean ;

   rc = connectNodeMaster( rgName, &dataDB ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect data node" ;
   rc = createStatisInfo( dataDB ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create statistics info" ;

   bson_init( &option ) ;
   bson_append_string( &option, "Type", "sessions" ) ;
   bson_finish( &option ) ;
   rc = sdbResetSnapshot( db, &option );
   bson_destroy( &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to reset snapshot" ;

   rc = isDataBaseSnapClean( dataDB, &isClean ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to check database snapshot" ;
   ASSERT_FALSE( isClean ) << "snapshot-database shouldn't be reset" ;
   rc = isSessionSnapClean( dataDB, &isClean ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to check session snapshot" ;
   ASSERT_TRUE( isClean ) << "snapshot-session has not been reset" ;

   rc = sdbResetSnapshot( db, NULL ) ;   
   ASSERT_EQ( SDB_OK, rc ) << "fail to reset snapshot when option is NULL" ;

   sdbDisconnect( dataDB ) ;
   sdbReleaseConnection( dataDB ) ;
}
