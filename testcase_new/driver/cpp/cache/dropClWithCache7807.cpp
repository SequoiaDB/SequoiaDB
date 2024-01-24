/**************************************************************
 * @Description: turn on cache and drop cl
 *               seqDB-7807 : turn on cache and drop cl by 
 *                            another connection
 * @Modify     : Suqiang Ling
 *               2017-09-11
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include <vector>
#include <sys/time.h>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"
#include "client.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

#define CACHE_TIME_INT 1000 /*millisecond*/

class turnOnCache7807 : public testBase 
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;

   void SetUp() 
   {
      INT32 rc = SDB_OK ;

      // turn on cache
      sdbClientConf conf ;
      conf.enableCacheStrategy = TRUE ;
      conf.cacheTimeInterval = CACHE_TIME_INT / 1000 ;
      rc = initClient( &conf );
      ASSERT_EQ( SDB_OK, rc ) << "fail to initClient" ;

      // connect and create cs, cl
      pCsName = "turnOnCache7807" ;
      pClName = "turnOnCache7807" ;
      sdbCollectionSpace cs ;
      sdbCollection cl ;
      rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() );
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect db" ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = cs.createCollection( pClName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;
   }

   void TearDown() 
   {
      if( shouldClear() )
      {
         INT32 rc = SDB_OK ;
         rc = db.dropCollectionSpace( pCsName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
      }
      db.disconnect() ;
   }
} ;

TEST_F( turnOnCache7807, dropCollection )
{
   INT32 rc = SDB_OK ;
   sdb db2 ; // another db to drop cs
   rc = db2.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() );
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect db2" ;

   sdbCollectionSpace cs ;
   rc = db2.getCollectionSpace( pCsName, cs ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs" ;

   rc = cs.dropCollection( pClName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl" ;
   db2.disconnect() ;

   ossSleep( CACHE_TIME_INT ) ;
   sdbCollectionSpace cs1 ; 
   rc = db.getCollectionSpace( pCsName, cs1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs" ;
   sdbCollection cl ;
   rc = cs1.getCollection( pClName, cl ) ;
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) ;
}
