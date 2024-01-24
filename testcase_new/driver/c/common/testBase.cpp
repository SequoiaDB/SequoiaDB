/**************************************************************
 * @Description: test base class
 * @Modify     : Liang xuewang 
 *               2017-09-17
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "arguments.hpp"
#include "testBase.hpp"

void testBase::SetUp()
{
   INT32 rc = SDB_OK ;
   forceClear = ARGS->forceClear() ;
   rc = sdbConnect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd(), &db ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
}

void testBase::TearDown()
{
   sdbDisconnect( db ) ;
   sdbReleaseConnection( db ) ;
}

BOOLEAN testBase::shouldClear()
{
   return ( !HasFailure() || forceClear ) ;
}
