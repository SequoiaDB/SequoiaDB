/**************************************************************
 * @Description: reset snapshot
 *               seqDB-12664 : reset snapshot
 * @Modify     : Suqiang Ling
 *               2017-09-11
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include <vector>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class getSnapshot12664 : public testBase 
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;

   void SetUp() 
   {
      testBase::SetUp() ;

      pCsName = "getSnapshot12664" ;
      pClName = "getSnapshot12664" ;
      sdbCollectionSpace cs ;
      sdbCollection cl ;

      INT32 rc = SDB_OK ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      BSONObj option = BSON( "ReplSize" << 0 ) ;
      rc = cs.createCollection( pClName, option, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;

      BSONObj doc = BSON( "a" << 1 ) ;
      rc = cl.insert( doc ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;
   }

   void TearDown() 
   {
      INT32 rc = SDB_OK ;
      if( shouldClear() )
      {
         rc = db.dropCollectionSpace( pCsName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
      }

      testBase::TearDown() ;
   }
} ;

TEST_F( getSnapshot12664, resetSnapshot )
{
   // get snapshot before reset
   INT32 rc = SDB_OK ;
   sdbCursor cursor ;
   BSONObj res ;

   rc = db.getSnapshot( cursor, SDB_SNAP_DATABASE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.next( res ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   INT32 totalInsert = ( INT32 )res.getField( "TotalInsert" ).number() ;
   ASSERT_NE( 0, totalInsert ) ;

   // reset snapshot
   rc = db.resetSnapshot() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   // get snapshot after reset
   rc = db.getSnapshot( cursor, SDB_SNAP_DATABASE ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.next( res ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;

   totalInsert = ( INT32 )res.getField( "TotalInsert" ).number() ;
   ASSERT_EQ( 0, totalInsert ) ;
}
