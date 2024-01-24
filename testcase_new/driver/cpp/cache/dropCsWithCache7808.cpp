/**************************************************************
 * @Description: turn on cache and drop cs
 *               seqDB-7808 : turn on cache and drop cs
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

class turnOnCache7808 : public testBase 
{
protected:
   const CHAR *pCsName ;

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
      pCsName = "turnOnCache7808" ;
      sdbCollectionSpace cs ;
      rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() );
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect db" ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
   }

   void TearDown() 
   {
      db.disconnect() ;
   }
} ;

TEST_F( turnOnCache7808, dropCollectionSpace )
{
   INT32 rc = SDB_OK ;
   rc = db.dropCollectionSpace( pCsName ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
   sdbCollectionSpace cs ;
   rc = db.getCollectionSpace( pCsName, cs ) ;
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc );
}
