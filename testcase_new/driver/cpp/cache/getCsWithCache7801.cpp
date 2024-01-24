/**************************************************************
 * @Description: turn on cache and get cs
 *               seqDB-7801 : turn on cache and get cs
 *               seqDB-7804 : turn on cache and get cs when 
 *                            it's cache time out
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

class turnOnCache7801 : public testBase 
{
protected:
   const CHAR *pCsName ;
   sdbCollectionSpace cs ;

   void SetUp() 
   {
      INT32 rc = SDB_OK ;

      // turn off cache
      sdbClientConf conf ;
      conf.enableCacheStrategy = FALSE ;
      rc = initClient( &conf );
      ASSERT_EQ( SDB_OK, rc ) << "fail to initClient" ;

      // connect and create cs
      pCsName = "turnOnCache7801" ;
      rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() );
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect db" ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;

      // turn on cache
      conf.enableCacheStrategy = TRUE ;
      conf.cacheTimeInterval = CACHE_TIME_INT / 1000 ;
      rc = initClient( &conf );
      ASSERT_EQ( SDB_OK, rc ) << "fail to initClient" ;
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

TEST_F( turnOnCache7801, getCollectionSpace )
{
   INT32 rc = SDB_OK ;
   time_t aliveTime1, aliveTime2, aliveTime3 ;

   // put cl to cache
   rc = db.getCollectionSpace( pCsName, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   aliveTime1 = db.getLastAliveTime() ;

   // get cl from cache
   ossSleep( CACHE_TIME_INT / 2 ) ;
   rc = db.getCollectionSpace( pCsName, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   aliveTime2 = db.getLastAliveTime() ;
   ASSERT_EQ( aliveTime1, aliveTime2 ) << "cache no expected cs" ;
   
   // get cs when timeout
   ossSleep( CACHE_TIME_INT ) ; // sleep utill cl not in cache 
   rc = db.getCollectionSpace( pCsName, cs ) ; // seqDB-7804
   ASSERT_EQ( SDB_OK, rc ) ;
   aliveTime3 = db.getLastAliveTime() ;
   ASSERT_NE( aliveTime3, aliveTime2 ) << "cl cache should be timeout" ;
}
