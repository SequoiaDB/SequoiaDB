/**************************************************************
 * @Description: parameter verification for initClient
 *               seqDB-12736 : initClient parameter verification
 * @Modify     : Suqiang Ling
 *               2017-09-11
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

TEST( initClientParamTest, initClient12736 )
{
   INT32 rc = SDB_OK ;
   rc = initClient( NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to initClient when sdbClientConf is NULL" ;
}
