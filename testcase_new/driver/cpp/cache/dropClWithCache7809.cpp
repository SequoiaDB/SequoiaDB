/**************************************************************
 * @Description: turn on cache and drop cl
 *               seqDB-7809 : turn on cache and drop cl
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

class turnOnCache7809 : public testBase 
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
      conf.cacheTimeInterval = 0 ;
      rc = initClient( &conf );
      ASSERT_EQ( SDB_OK, rc ) << "fail to initClient" ;

      // connect and create cs, cl
      pCsName = "turnOnCache7809" ;
      pClName = "turnOnCache7809" ;
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

TEST_F( turnOnCache7809, dropCollection )
{
   INT32 rc = SDB_OK ;

   sdbCollectionSpace cs ;
   rc = db.getCollectionSpace( pCsName, cs ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs" ;

   rc = cs.dropCollection( pClName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl" ;

   sdbCollection cl ;
   rc = cs.getCollection( pClName, cl ) ;
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) ;
}
