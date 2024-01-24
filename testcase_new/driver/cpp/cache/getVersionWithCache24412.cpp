/**************************************************************
 * @Description: 开启缓存，获取集合版本号
 *               seqDB-24412 : 开启缓存，获取集合版本号
 * @Modify     : wenjing wang 
 *               2021-09-29
 ***************************************************************/
#include <gtest/gtest.h>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"
#include "client.hpp"

using namespace bson ;
using namespace sdbclient ;

class getVersionWithCache24412 : public testBase 
{
protected:
   sdbCollectionSpace cs ;
   const char *pCsName ;
   const char *pClName1 ;
   const char *pClName2 ;
 

   void SetUp() 
   {
      INT32 rc = SDB_OK ;

      sdbCollection cl ;
      // turn on cache
      sdbClientConf conf ;
      conf.enableCacheStrategy = TRUE ;
      conf.cacheTimeInterval = 1 ;
      rc = initClient( &conf );
      ASSERT_EQ( SDB_OK, rc ) << "fail to initClient" ;

      // connect and create cs
      pCsName = "tis_sit" ;
      rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() );
      ASSERT_EQ( SDB_OK, rc ) << "fail to connect db" ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
	  
      pClName1="t_policy_problem_handle_file";
      rc = cs.createCollection( pClName1, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl:" << pClName1 ;
      pClName2="t_policy_problem" ;
      rc = cs.createCollection( pClName2, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl:" <<  pClName2;
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

TEST_F( getVersionWithCache24412, getVersion )
{
    INT32 rc ;
    sdbCollection cl1;
    sdbCollection cl2;
    sdbCollection cl3;
    rc = cs.getCollection(pClName1, cl1);
	
    ASSERT_EQ( SDB_OK, rc ) << "fail to get cl: "<<  pClName1;
    int version = cl1.getVersion();
      

    rc = cs.getCollection(pClName2, cl2);
    long long count = 0;
    cl2.disableCompression();
    cl2.getCount(count);
    ASSERT_EQ( SDB_OK, rc ) << "fail to get cl: "<<  pClName2;
    int version2 = cl2.getVersion();
    
    rc = rc ? rc : cs.getCollection(pClName1, cl3);
    int version3 = cl3.getVersion();
    ASSERT_EQ( version3, version );
    ASSERT_NE( version2, version ) ;
}
