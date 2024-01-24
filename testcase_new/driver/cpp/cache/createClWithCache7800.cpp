/**************************************************************
 * @Description: turn on cache and createCL
 *               seqDB-7800 : turn on cache and createCL
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

class turnOnCache7800 : public testBase 
{
protected:
   const CHAR *pCsName ;
   sdbCollectionSpace cs ;

   void SetUp() 
   {
      INT32 rc = SDB_OK ;

      // turn on cache
      sdbClientConf conf ;
      conf.enableCacheStrategy = TRUE ;
      conf.cacheTimeInterval = 0 ;
      rc = initClient( &conf );
      ASSERT_EQ( SDB_OK, rc ) << "fail to initClient" ;

      // connect and create cs
      pCsName = "turnOnCache7800" ;
      rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() );
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect db" ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
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

TEST_F( turnOnCache7800, createCollection )
{
   sdbCollection cl ;
   INT32 rc = SDB_OK ;
   rc = cs.createCollection( "turnOnCache7800", cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;
}
