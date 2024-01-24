/**************************************************************
 * @Description: turn on cache and unload load cs
 *               seqDB-14693:开启缓存，unload/loadCS
 * @Modify     : Liang xuewang
 *               2018-03-13
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include <sys/time.h>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

#define CACHE_TIME_INT 2000 /*millisecond*/

class loadUnloadCsWithCache14693 : public testBase 
{
protected:
   const CHAR* csName ;
   sdbCollectionSpace cs ;

   void SetUp() 
   {
      INT32 rc = SDB_OK ;
      
      // turn on cache
      sdbClientConf conf ;
      conf.enableCacheStrategy = TRUE ;
      conf.cacheTimeInterval = CACHE_TIME_INT / 1000 ;
      rc = initClient( &conf );
      ASSERT_EQ( SDB_OK, rc ) << "fail to initClient" ;

      // connect and create cs
      csName = "loadUnloadCsWithCacheTestCs14693" ;
      testBase::SetUp() ;
      rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
   }

   void TearDown() 
   {
      if( shouldClear() )
      {
         INT32 rc = SDB_OK ;
         rc = db.dropCollectionSpace( csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
      }
      testBase::TearDown() ;
   }

   INT32 getTimeOfGetCs( sdb& db, const CHAR* csName, clock_t* diff )
   {
      INT32 rc = SDB_OK ;
      timeval begin, end ;
      sdbCollectionSpace tmpCs ;
      gettimeofday( &begin, NULL ) ;
      rc = db.getCollectionSpace( csName, cs ) ;
      CHECK_RC( SDB_OK, rc, "fail to getCS" ) ;
      gettimeofday( &end, NULL ) ;
      *diff = ( end.tv_sec - begin.tv_sec ) * 1000 * 1000 + 
              ( end.tv_usec - begin.tv_usec ) ;  // micro seconds
   done:
      return rc ;
   error:
      goto done ;
   }
} ;

TEST_F( loadUnloadCsWithCache14693, loadUnloadCS )
{
   INT32 rc = SDB_OK ;

   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   // before unloadCS, get cs from cache
   clock_t diff1 = 0 ;
   rc = getTimeOfGetCs( db, csName, &diff1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "before unloadCS, getCS takes " << diff1 << "us" << endl ;

   // unload cs
   rc = db.unloadCS( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to unloadCS" ;
   
   // after unloadCS, get cs from coord 
   clock_t diff2 = 0 ;
   rc = getTimeOfGetCs( db, csName, &diff2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "after unloadCS, getCS takes " << diff2 << "us" << endl ;
   // ASSERT_LT( diff1, diff2 ) << "fail to check unloadCS" ;

   // unloadCS again then load cs
   rc = db.unloadCS( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to unloadCS" ;
   rc = db.loadCS( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to loadCS" ;

   // after loadCS, get cs from cache
   clock_t diff3 = 0 ;
   rc = getTimeOfGetCs( db, csName, &diff3 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "after loadCS, getCS takes " << diff3 << "us" << endl ;
   // ASSERT_LT( diff3, diff2 ) << "fail to check loadCS" ;
}