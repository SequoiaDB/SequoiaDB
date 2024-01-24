/**************************************************************************
 * @Description:   seqDB-13424: truncateLob接口参数校验 
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

class lockParamVerify13422 : public testBase
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   OID oid ;
   INT32 bufSize ;
   
   void SetUp()
   {
      testBase::SetUp() ;

      INT32 rc = SDB_OK ;
      pCsName = "lockParamVerify13422" ;
      pClName = "lockParamVerify13422" ;
      sdbLob lob ;
      const CHAR *buf = "0123456789ABCDEabcde" ;
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

TEST_F( lockParamVerify13422, lockLob )
{
   INT32 rc = SDB_OK ;
   const INT64 maxInt64 = 0x7FFFFFFFFFFFFFFFLL ;
   const INT64 minInt64 = -9223372036854775807LL-1 ;
   OID inexistOid( "5a114afdf980000000000000" ) ;
   INT32 lobSize = 0 ;
   
   // illegal parameter
   rc = cl.truncateLob( inexistOid, 8 ) ;
   ASSERT_EQ( SDB_FNE, rc ) << "rc must be SDB_INVALIDARG, when oid is inexistent" ;
   rc = cl.truncateLob( oid, -1 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "rc must be SDB_INVALIDARG, when length is -1" ;
   rc = cl.truncateLob( oid, minInt64 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) << "rc must be SDB_INVALIDARG, when length is minInt64" ;

   // legal parameter
   rc = cl.truncateLob( oid, maxInt64 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to truncateLob when length is maxInt64" ;
   rc = cl.truncateLob( oid, bufSize / 2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to truncateLob" ;
   rc = getLobSize( &lobSize ) ;
   ASSERT_EQ( bufSize / 2, lobSize ) << "wrong lob size" ;
}
