/**************************************************************
 * @Description: turn on cache and get cl
 *               seqDB-7802 : turn on cache and get cl
 *               seqDB-7805 : turn on cache and get cl when 
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

#define CACHE_TIME_INT 3000 /*millisecond*/

class turnOnCache7802 : public testBase 
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;

   void SetUp() 
   {
      INT32 rc = SDB_OK ;

      // turn off cache
      sdbClientConf conf ;
      conf.enableCacheStrategy = FALSE ;
      rc = initClient( &conf );
      ASSERT_EQ( SDB_OK, rc ) << "fail to initClient" ;

      // connect and create cs
      pCsName = "turnOnCache7802" ;
      pClName = "turnOnCache7802" ;
      rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() );
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect db" ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = cs.createCollection( pClName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;

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

TEST_F( turnOnCache7802, getCollection )
{
   INT32 rc = SDB_OK ;
   time_t aliveTime1, aliveTime2, aliveTime3 ;

   // put cl to cache
   rc = cs.getCollection( pClName, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   aliveTime1 = db.getLastAliveTime() ;

   // get cl from cache
   ossSleep( 1000 ) ; 
   rc = cs.getCollection( pClName, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   aliveTime2 = db.getLastAliveTime() ;
   ASSERT_EQ( aliveTime1, aliveTime2 ) << "cache no expected cl" ;
   
   // get cl when timeout
   ossSleep( CACHE_TIME_INT ) ; // sleep utill cl not in cache 
   rc = cs.getCollection( pClName, cl ) ; // seqDB-7805
   ASSERT_EQ( SDB_OK, rc ) ;
   aliveTime3 = db.getLastAliveTime() ;
   ASSERT_NE( aliveTime3, aliveTime2 ) << "cl cache should be timeout" ;
}
