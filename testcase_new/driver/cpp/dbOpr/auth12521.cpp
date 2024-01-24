/**************************************************************
 * @Description: test authentication
 *               seqDB-12521 : create/drop user
 * @Modify     : Suqiang Ling
 *               2017-09-04
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class auth12521 : public testBase{} ;

// to test authentication, create 2 users, then remove one of them.
// if no user, any user and passwd is permitted.
TEST_F( auth12521, createAndDropUser )
{
   if( isStandalone( db ) )
   {
      cout << "skip this test for standalone" << endl ; 
      return ;
   }

   // create user1
   INT32 rc = SDB_OK ;
   const CHAR* newUser1 = "sdbUser1" ;
   const CHAR* newPasswd1 = "sdbPasswd1" ;
   rc = db.createUsr( newUser1, newPasswd1 ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to create user1" ;

   // create user2
   const CHAR* newUser2 = "sdbUser2" ;
   const CHAR* newPasswd2 = "sdbPasswd2" ;
   rc = db.createUsr( newUser2, newPasswd2 ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to create user2" ;
   db.disconnect() ;

   // check create ok
   const CHAR* wrongUser = "sdbUser3" ;
   const CHAR* wrongPasswd = "sdbPasswd3" ;
   rc = db.connect( ARGS->hostName(), ARGS->svcName(), wrongUser, wrongPasswd ) ;
   ASSERT_EQ( SDB_AUTH_AUTHORITY_FORBIDDEN, rc ) << "fail to use authentication" ;

   // remove user1
   rc = db.connect( ARGS->hostName(), ARGS->svcName(), newUser1, newPasswd1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to use user1" ;
   rc = db.removeUsr( newUser1, newPasswd1 ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to remove user1" ; 
   db.disconnect() ;

   // check remove ok
   rc = db.connect( ARGS->hostName(), ARGS->svcName(), newUser1, newPasswd1 ) ;
   ASSERT_EQ( SDB_AUTH_AUTHORITY_FORBIDDEN, rc ) << "fail to remove user1" ; 
   
   // remove user2
   rc = db.connect( ARGS->hostName(), ARGS->svcName(), newUser2, newPasswd2 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to use user2" ;
   rc = db.removeUsr( newUser2, newPasswd2 ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to remove user2" ; 
}
