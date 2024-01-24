/**************************************************************
 * @Description: test sdbCollectionSpace without init
 *               seqDB-14036:sdbCollectionSpace对象未创建时调用方法
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

class sdbCollectionSpace14036 : public testBase 
{
protected:
   sdbCollectionSpace cs ;
   void SetUp() 
   {
   }
   void TearDown()
   {
   }
} ;

TEST_F( sdbCollectionSpace14036, opCS )
{
   // test all function of sdbCollectionSpace

   INT32 rc = SDB_OK ;
   const CHAR *pClName = "cl14036" ;
   sdbCollection cl ;
   rc = cs.getCollection( pClName, cl ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get cl shouldn't succeed" ;
   BSONObj option ;
   rc = cs.createCollection( pClName, option, cl ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "create cl(1) shouldn't succeed" ;
   rc = cs.createCollection( pClName, cl ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "create cl(2) shouldn't succeed" ;
   rc = cs.dropCollection( pClName ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "drop cl shouldn't succeed" ;
   
   EXPECT_FALSE( cs.getCSName() ) << "getCSName should be NULL" ;
   const CHAR* oldName = "oldCl14036" ;
   const CHAR* newName = "newCl14036" ;
   rc = cs.renameCollection( oldName, newName ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "renameCollection shouldn't succeed" ;
}
