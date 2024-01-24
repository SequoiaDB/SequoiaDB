/**************************************************************
 * @Description: turn on cache and listLobPieces
 *               seqDB-14696:开启缓存，listLobPieces
 * @Modify     : Liang xuewang
 *               2018-03-14
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

class listLobPiecesWithCache14696 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   const CHAR* clFullName ;
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

      testBase::SetUp() ;
      csName = "listLobPiecesTestCs14696" ;
      clName = "listLobPiecesTestCl14696" ;
      clFullName = "listLobPiecesTestCs14696.listLobPiecesTestCl14696" ;
      rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }

   void TearDown() 
   {
      INT32 rc = SDB_OK ;
      rc = db.dropCollectionSpace( csName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
      testBase::TearDown() ;
   }

   INT32 getTimeOfGetCl( sdb& db, const CHAR* clFullName, clock_t* diff )
   {
      INT32 rc = SDB_OK ;
      timeval begin, end ;
      sdbCollection tmpCl ;
      gettimeofday( &begin, NULL ) ;
      rc = db.getCollection( clFullName, tmpCl ) ;
      CHECK_RC( SDB_OK, rc, "fail to getCL" ) ;
      gettimeofday( &end, NULL ) ;
      *diff = ( end.tv_sec - begin.tv_sec ) * 1000 * 1000 + 
              ( end.tv_usec - begin.tv_usec ) ;  // micro seconds
   done:
      return rc ;
   error:
      goto done ;
   }
} ;

TEST_F( listLobPiecesWithCache14696, listLobPieces )
{
   INT32 rc = SDB_OK ;

   // wait cache timeout then getCL, get cl from coord
   ossSleep( CACHE_TIME_INT + 1000 ) ;
   clock_t diff1 ;
   rc = getTimeOfGetCl( db, clFullName, &diff1 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "after cache timeout, it takes " << diff1 << "us to getCL" << endl ;

   // wait cache timeout again then listLobPieces
   ossSleep( CACHE_TIME_INT + 1000 ) ;
   sdbCursor cursor ;
   rc = cl.listLobPieces( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to listLobPieces" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // after listLobPieces then getCL, get cl from cache
   clock_t diff2 ;
   rc = getTimeOfGetCl( db, clFullName, &diff2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "after listLobPieces, it takes " << diff2 << "us to getCL" << endl ;

   ASSERT_LT( diff2, diff1 ) << "fail to check listLobPieces with cache" ;
}
