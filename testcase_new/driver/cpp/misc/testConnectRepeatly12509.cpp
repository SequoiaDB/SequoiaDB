/**************************************************************
 * @Description: connect and disconnect repeatly(cover all the 
 *               way of connect)
 *               seqDB-12509 : connect and disconnect repeatly
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

class testConnectRepeatly12509 : public testBase 
{
protected:
   void SetUp() {} ;
   void TearDown() {} ;
} ;

TEST_F( testConnectRepeatly12509, test )
{
   INT32 rc = SDB_OK ;
   BOOLEAN isValid = FALSE ;
   BOOLEAN isClosed = FALSE ;

   rc = db.connect( ARGS->hostName(), ARGS->port() ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.isValid( &isValid ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, isValid ) ;

   isClosed = db.isClosed() ;
   ASSERT_EQ( FALSE, isClosed ) ;
   db.disconnect() ;
   isClosed = db.isClosed() ;
   ASSERT_EQ( TRUE, isClosed ) ;
   rc = db.connect( ARGS->hostName(), ARGS->port(), ARGS->user(), ARGS->passwd() ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.isValid( &isValid ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, isValid ) ;
   db.disconnect() ;

   rc = db.connect( ARGS->hostName(), ARGS->svcName() ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.isValid( &isValid ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, isValid ) ;
   db.disconnect() ;

   // connect repeatly
   rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.isValid( &isValid ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, isValid ) ;

   const CHAR *pConnAddrs[1] ;
   pConnAddrs[0] = ARGS->coordUrl() ;
   rc = db.connect( pConnAddrs, 1, ARGS->user(), ARGS->passwd() ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.isValid( &isValid ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_EQ( TRUE, isValid ) ;

   // disconnect repeatly
   db.disconnect() ;
   db.disconnect() ;
}
