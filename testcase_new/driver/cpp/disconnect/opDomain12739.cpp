/**************************************************************
 * @Description: opreate domain object after disconnect
 *               seqDB-12739 : opreate domain object after disconnect
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

class opDomain12739 : public testBase 
{
protected:
   sdbDomain domain ;
   const CHAR *pDomainName ;

   void SetUp() 
   {
      testBase::SetUp() ;
      if( isStandalone( db ) ) 
         return ;

      INT32 rc = SDB_OK ;
      pDomainName = "domain12739" ;
      BSONObj option ;
      rc = db.createDomain( pDomainName, option, domain ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create domain" ;
      db.disconnect() ;
   }

   void TearDown()
   {
      if( !isStandalone( db ) && shouldClear() )
      {
         INT32 rc = SDB_OK ;
         rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to connect db" ;
         rc = db.dropDomain( pDomainName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop domain" ;
      }
      testBase::TearDown() ;
   }
} ;

TEST_F( opDomain12739, opDoamin )
{
   if( isStandalone( db ) )
   {
      cout << "skip this test for standalone" << endl ; 
      return ;
   }

   // test all interfaces of class sdbDomain except getName()
   // in the order of c++ api doc

   INT32 rc = SDB_OK ;
   BSONObj option ;
   rc = domain.alterDomain( option ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "alter domain shouldn't succeed" ;
   sdbCursor cursor ;
   rc = domain.listCollectionSpacesInDomain( cursor ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "list cs in domain shouldn't succeed" ;
   rc = domain.listCollectionsInDomain( cursor ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "list cl in domain shouldn't succeed" ;
   rc = domain.listReplicaGroupInDomain( cursor ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "listReplicaGroupInDomain shouldn't succeed" ;
}
