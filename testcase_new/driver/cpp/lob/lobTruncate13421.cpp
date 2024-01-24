/**************************************************************************
 * @Description:   seqDB-13421: truncate删除lob数据 
 * @Modify:        Suqiang Ling
 *                 2017-11-17
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class lobTruncate13421 : public testBase
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   OID oid ;
   const CHAR *buf ;
   INT32 bufSize ;
   
   void SetUp()
   {
      testBase::SetUp() ;

      INT32 rc = SDB_OK ;
      pCsName = "lobTruncate13421" ;
      pClName = "lobTruncate13421" ;
      sdbLob lob ;
      buf = "0123456789ABCDEabcde" ;
      bufSize = strlen( buf ) ;

      // create cs, cl
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = cs.createCollection( pClName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;

      // create a lob
      rc = cl.createLob( lob ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;
      rc = lob.write( buf, bufSize ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
      rc = lob.getOid( oid ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get oid" ;
      rc = lob.close() ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = db.dropCollectionSpace( pCsName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << pCsName ;
      } 
      testBase::TearDown() ;
   }

   INT32 getLobSize( INT32 *lobSize )
   {
      INT32 rc = SDB_OK ;
      sdbCursor cursor ;
      BSONObj obj ;
      OID currOid ;
      BOOLEAN foundOid = FALSE ;

      rc = cl.listLobs( cursor ) ;
      CHECK_RC( SDB_OK, rc, "%s", "fail to list lobs" ) ;

      // find the specified size corresponsing the oid
      while( !( rc=cursor.next( obj ) ) )
      {
         currOid = obj.getField( "Oid" ).OID() ;
         if( currOid == oid )
         {
            *lobSize = ( INT32 )obj.getField( "Size" ).Long() ;
            foundOid = TRUE ;
            break ;
         }
      }
      if( rc != SDB_DMS_EOC && rc != SDB_OK )
      {
         CHECK_RC( SDB_OK, rc, "%s", "fail to get cursor next" ) ;
      }
      if( !foundOid )
      {
         cout << "no such oid in list" << endl ;
         rc = SDB_TEST_ERROR ;
         goto error ;
      }
   done:
      cursor.close() ;
      return rc ;
   error: 
      goto done ;
   }
} ;

TEST_F( lobTruncate13421, eqLobSize )
{
   INT32 rc = SDB_OK ;
   INT32 actLobSize ;

   rc = cl.truncateLob( oid, bufSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to truncate" ;
   rc = getLobSize( &actLobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get lob size" ;
   ASSERT_EQ( bufSize, actLobSize ) << "wrong lob size" ;
}

TEST_F( lobTruncate13421, gtLobSize )
{
   INT32 rc = SDB_OK ;
   INT32 actLobSize ;

   rc = cl.truncateLob( oid, bufSize * 2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to truncate" ;
   rc = getLobSize( &actLobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get lob size" ;
   ASSERT_EQ( bufSize, actLobSize ) << "wrong lob size" ;
}

TEST_F( lobTruncate13421, ltLobSize )
{
   INT32 rc = SDB_OK ;
   INT32 actLobSize ;

   rc = cl.truncateLob( oid, bufSize / 2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to truncate" ;
   rc = getLobSize( &actLobSize ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get lob size" ;
   ASSERT_EQ( bufSize / 2, actLobSize ) << "wrong lob size" ;
}
