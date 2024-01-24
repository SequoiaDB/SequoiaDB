/*****************************************************************
 * @Description: testcase for connectionpool
 *               seqDB-9577:用新版本的驱动替换旧版本驱动的动态库
 *               手工验证，不加入scons编译脚本    
 * @Modify:      Liangxw
 *               2017-09-05
 *****************************************************************/
#include <client.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include "connpool_common.hpp"

using namespace sdbclient ;
using namespace std ;
using namespace bson ;

class libTest9577 : public testBase
{
protected:
   void SetUp()
   {
      testBase::SetUp() ;
   }
   void TearDown()
   {
      testBase::TearDown() ;
   }
} ;

TEST_F( libTest9577, sdb9577 )
{
   INT32 rc = SDB_OK ;

   // create cs cl
   sdbCollectionSpace cs ;
   const CHAR* csName = "testLibCs9577" ;
   sdbCollection cl ;
   const CHAR* clName = "testLibCl9577" ;
   rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
   ASSERT_EQ( rc, SDB_OK ) ;

   // insert
   BSONObj doc = BSON( "a" << "first" ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   // query and check result
   sdbCursor cursor ;
   BSONObj sel = BSON( "a" << "" ) ;
   rc = cl.query( cursor, doc, sel ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   BSONObj obj ;
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get current" ;
   ASSERT_STREQ( "{ \"a\": \"first\" }", obj.toString().c_str() ) << "fail to check query result" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // delete and check result
   rc = cl.del( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to delete" ;
   rc = cl.query( cursor, doc, sel ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = cursor.current( obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to check query result after delete" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // drop cs and disconnect
   rc = db.dropCollectionSpace( csName ) ;
   ASSERT_EQ( rc, SDB_OK ) << "fail to drop cs " << csName ;
}
