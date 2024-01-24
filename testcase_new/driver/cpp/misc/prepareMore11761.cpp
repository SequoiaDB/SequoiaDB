/***************************************************************************
 * @Description: testcase for Jira Questionaire
 *               seqDB-2504:c++驱动flag增加QUERY_PREPARE_MORE
 *               seqDB-11761:query接口flag参数取值为prepare more进行查询
 * @Modify:      Liangxw
 *               2017-09-27
 ***************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class prepareMoreTest11761 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;

   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "prepareMoreTestCs11761" ;
      clName = "prepareMoreTestCl11761" ;
      rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      if( shouldClear() )
      {
         rc = db.dropCollectionSpace( csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      }
      testBase::TearDown() ;
   }
} ;

TEST_F( prepareMoreTest11761, prepareMore11761 )
{
   INT32 rc = SDB_OK ;

   // insert doc
   BSONObj doc = BSON( "a" << 1 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   // query doc 
   sdbCursor cursor ;
   rc = cl.query( cursor, doc, _sdbStaticObject, _sdbStaticObject,
                  _sdbStaticObject, 0, -1, QUERY_PREPARE_MORE ) ; 
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   INT32 value = obj.getField( "a" ).Int() ;
   ASSERT_EQ( 1, value ) << "fail to check query result" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}
