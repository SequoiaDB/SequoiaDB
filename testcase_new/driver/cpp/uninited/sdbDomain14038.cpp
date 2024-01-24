/**************************************************************
 * @Description: test sdbDomain without init
 *               seqDB-14038:sdbDomain对象未创建时调用方法
 * @Modify     : Liang xuewang
 *               2018-01-09
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class sdbDomain14038 : public testBase 
{
protected:
   sdbDomain domain ;
   void SetUp() 
   {
   }
   void TearDown()
   {
   }
} ;

TEST_F( sdbDomain14038, opDoamin )
{
   // test all function of sdbDomain

   INT32 rc = SDB_OK ;

   EXPECT_FALSE( domain.getName() ) << "getName should be NULL" ;
   BSONObj option ;
   rc = domain.alterDomain( option ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "alter domain shouldn't succeed" ;
   sdbCursor cursor ;
   rc = domain.listCollectionSpacesInDomain( cursor ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "list cs in domain shouldn't succeed" ;
   rc = domain.listCollectionsInDomain( cursor ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "list cl in domain shouldn't succeed" ;
   rc = domain.listReplicaGroupInDomain( cursor ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "list rg in domain shouldn't succeed" ;
}
