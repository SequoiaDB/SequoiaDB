/**************************************************************
 * @Description: test case for c driver
 *			        concurrent test with multi domain
 *               sync test, no need to check standalone
 * @Modify:      Liang xuewang Init
 *				     2016-11-10
 **************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "impWorker.hpp"
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace import ;

#define ThreadNum 5

class concurrentDomainTest : public testBase
{
protected:
   sdbDomainHandle domain[ ThreadNum ] ;
   CHAR* domainName[ ThreadNum ] ;
   const CHAR* rgName ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;

      vector<string> groups ;
      rc = getGroups( db, groups ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get data groups" ;
      rgName = groups[0].c_str() ;

      for( INT32 i = 0;i < ThreadNum;i++ )
      {
         CHAR tmp[100] = { 0 } ;
         sprintf( tmp, "%s%d", "concurrentTestDomain", i ) ;
         domainName[i] = strdup( tmp ) ;
      }

      // option { AutoSplit: true, Groups: [ rgName ] }
      bson option ;
      bson_init( &option ) ;
      bson_append_bool( &option, "AutoSplit", true ) ;
      bson_append_start_array( &option, "Groups" ) ;
      bson_append_string( &option, "0", rgName ) ;
      bson_append_finish_array( &option ) ;
      bson_finish( &option ) ;

      // create domain with option
      for( INT32 i = 0;i < ThreadNum;i++ )
      {
         rc = sdbCreateDomain( db, domainName[i], &option, &domain[i] ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to create doamin " << domainName[i] ;
      }
      bson_destroy( &option ) ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      if( !HasFailure() )
      {              
         for( INT32 i = 0;i < ThreadNum;i++ )
         {        
            rc = sdbDropDomain( db, domainName[i] ) ;
            ASSERT_EQ( SDB_OK, rc ) << "fail to drop domain " << domainName[i] ;
            sdbReleaseDomain( domain[i] ) ;
            free( domainName[i] ) ;
         }
      }
      testBase::TearDown() ;
   }
} ;

class ThreadArg : public WorkerArgs
{
public:
   INT32 id ;
   sdbDomainHandle dom ; 
} ;

void func_domain( ThreadArg* arg )
{
   INT32 i = arg->id ;
   sdbDomainHandle dom = arg->dom ;
   INT32 rc = SDB_OK ;

   bson option ;
   bson_init( &option ) ;
   bson_append_bool( &option, "AutoSplit", false ) ;
   bson_finish( &option ) ;
   rc = sdbAlterDomain( dom, &option ) ;
   bson_destroy( &option ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to alter domain " << i ;
}

TEST_F( concurrentDomainTest, domain )
{
   Worker * workers[ThreadNum] ;
   ThreadArg arg[ThreadNum] ;
   for( int i = 0;i < ThreadNum;++i )
   {
      arg[i].id = i ;
      arg[i].dom = domain[i] ;
      workers[i] = new Worker( (WorkerRoutine)func_domain, &arg[i], false ) ;
      workers[i]->start() ;
   }
   for( int i = 0;i < ThreadNum;++i )
   {
      workers[i]->waitStop() ;
      delete workers[i] ;
   }
}
