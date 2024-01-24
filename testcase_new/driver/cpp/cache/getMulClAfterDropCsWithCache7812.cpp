/**************************************************************
 * @Description: turn on cache and get multi cl after drop cs
 *               seqDB-7812 : turn on cache and get multi cl 
 *                            after drop cs
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

#define MAX_CL_NAME_LEN 256
#define CL_NUM 5

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class turnOnCache7812 : public testBase 
{
protected:
   const CHAR *pCsName ;
   sdbCollectionSpace cs ;
   CHAR clNameArr[CL_NUM][MAX_CL_NAME_LEN] ;

   void SetUp() 
   {
      INT32 rc = SDB_OK ;

      // init client
      sdbClientConf conf ;
      conf.enableCacheStrategy = TRUE ;
      conf.cacheTimeInterval = 0 ;
      rc = initClient( &conf );
      ASSERT_EQ( SDB_OK, rc ) << "fail to initClient" ;

      // connect db
      rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() );
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect db" ;

      // create cs
      pCsName = "turnOnCache7812" ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;

      // create multi cl of cs
      const CHAR *pClNameBase = "turnOnCache7812" ;
      for( INT32 i = 0 ; i < CL_NUM ; ++i )
      {
         sdbCollection cl ;
         generateClName( pClNameBase, i, clNameArr[i] ) ;
         rc = cs.createCollection( clNameArr[i], cl ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;
      }
   }

   void TearDown() 
   {
      db.disconnect() ;
   }

   void generateClName( const CHAR *pClNameBase, INT32 i, CHAR *resultClName )
   {
      strcpy( resultClName, pClNameBase ) ;
      strcat( resultClName, "_" ) ;
      CHAR digitCharArr[4] ; // max cl num is 4096, so '4' is enough
      sprintf( digitCharArr, "%d", i ) ;
      strcat( resultClName, digitCharArr ) ;
   }
} ;

TEST_F( turnOnCache7812, getMulClAfterDropCs )
{
   INT32 rc = SDB_OK ;

   rc = db.dropCollectionSpace( pCsName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   INT32 expectRes = SDB_DMS_NOTEXIST ;
   if( isStandalone( db ) )
   {
      expectRes = SDB_DMS_CS_NOTEXIST ;
   }
   
   sdbCollection cl ;
   for( INT32 i = 0 ; i < CL_NUM ; ++i )
   {
      rc = cs.getCollection( clNameArr[i], cl ) ;
      ASSERT_EQ( expectRes, rc ) ;

      string clFullNameStr = string( pCsName ) + "." + string( clNameArr[i] ) ;
      rc = db.getCollection( clFullNameStr.c_str(), cl ) ;
      ASSERT_EQ( expectRes, rc ) ;
   }
}
