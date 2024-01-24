/**************************************************************
 * @Description: turn on cache and operate cl
 *               seqDB-7810 : turn on cache and operate cl to 
 *                            update timestamp
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

#define CACHE_TIME_INT 2000 /*millisecond*/

class turnOnCache7810 : public testBase 
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;

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
      pCsName = "turnOnCache7810" ;
      pClName = "turnOnCache7810" ;
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

TEST_F( turnOnCache7810, testUpdateTimeStamp )
{
   INT32 rc = SDB_OK ;
   time_t aliveTime1, aliveTime2 ;
   
   // put cl to cache
   rc = cs.getCollection( pClName, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // update timestamp
   ossSleep( CACHE_TIME_INT / 2 ) ;
   BSONObj doc = BSON( "a" << 1 ) ;
   rc = cl.insert( doc ) ;
   aliveTime1 = db.getLastAliveTime() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ossSleep( CACHE_TIME_INT / 2 ) ;

   // get cl from cache
   rc = cs.getCollection( pClName, cl ) ;
   aliveTime2 = db.getLastAliveTime() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( aliveTime1, aliveTime2 ) ;
}
