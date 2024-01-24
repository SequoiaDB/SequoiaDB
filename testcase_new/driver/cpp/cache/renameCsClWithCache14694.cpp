/**************************************************************
 * @Description: turn on cache and rename cs cl
 *               seqDB-14694:开启缓存，renameCS/CL
 *               rename cs cl only support standalone mode
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

class renameCsClWithCache14694 : public testBase 
{
protected:
   void SetUp() 
   {
      INT32 rc = SDB_OK ;
      
      // turn on cache
      sdbClientConf conf ;
      conf.enableCacheStrategy = TRUE ;
      conf.cacheTimeInterval = CACHE_TIME_INT / 1000 ;
      rc = initClient( &conf );
      ASSERT_EQ( SDB_OK, rc ) << "fail to initClient" ;

      testBase::SetUp() ;
   }

   void TearDown() 
   {
      testBase::TearDown() ;
   }
} ;

TEST_F( renameCsClWithCache14694, renameCsCl )
{
   INT32 rc = SDB_OK ;

   if( !isStandalone( db ) )
   {
      cout << "Run mode is not standalone" << endl ;
      return ;
   }
      
   // create cs cl
   const CHAR* oldCsName = "oldTestCs14694" ;
   const CHAR* newCsName = "newTestCs14694" ;
   const CHAR* oldClName = "oldTestCl14694" ;
   const CHAR* newClName = "newTestCl14694" ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   rc = createNormalCsCl( db, cs, cl, oldCsName, oldClName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   // before renameCS, it's ok to getcs with old name
   rc = db.getCollectionSpace( oldCsName, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs" ;

   // rename cs
   rc = db.renameCollectionSpace( oldCsName, newCsName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to renameCS" ;

   // after renameCS, getcs with old name is wrong
   rc = db.getCollectionSpace( oldCsName, cs ) ;
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc ) << "fail to test getcs with oldCsName" ;
   rc = db.getCollectionSpace( newCsName, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getcs with newCsName" ;

   // before renameCL, it's ok to getcl with old name
   rc = cs.getCollection( oldClName, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl" ;

   // rename cl
   rc = cs.renameCollection( oldClName, newClName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to rename cl" ;

   // after renameCL, getcl with old name is wrong
   rc = cs.getCollection( oldClName, cl ) ;
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) << "fail to test getcl with oldClName" ;
   rc = cs.getCollection( newClName, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getcl with newClName" ;
   
   rc = db.dropCollectionSpace( newCsName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
}
