/**************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-2507
 *               seqDB-11754:构造SdbDate数据并插入查询
 * @Modify:      Liang xuewang Init
 *			 	     2017-02-28
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class dateTest11754 : public testBase
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
      csName = "dateTestCs11754" ;
      clName = "dateTestCl11754" ;
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

TEST_F( dateTest11754, dateMills )
{
   INT32 rc = SDB_OK ;

   long long mills[] = {
      -62167248000000l,   // 0000-01-01 00:00:00
      253402271999000l,   // 9999-12-31 23:59:59
      -62167248001000l,   // -0001-12-31 23:59:59 
      253402272000000l    // 10000-01-01 00:00:00
   } ;
   for( INT32 i = 0;i < sizeof(mills)/sizeof(mills[0]);i++ )
   {
      BSONObjBuilder b ;
      BSONObj obj, res ;
      BSONObj sel = BSON( "myDate" << "" ) ;
      sdbCursor cursor ;

      rc = cl.truncate() ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to truncate" ;

      Date_t dt( mills[i] ) ;
      b.appendDate( "myDate", dt ) ;
      obj = b.obj() ;

      rc = cl.insert( obj ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to insert cl, mills: " << mills[i] ;

      rc = cl.query( cursor, _sdbStaticObject, sel ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to query cl, mills: " << mills[i] ;

      rc = cursor.next( res ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get next, mills: " << mills[i] ;

      BSONElement ele = res.getField( "myDate" ) ;
      BSONType type = ele.type() ;
      ASSERT_EQ( Date, type ) << "fail to check date type" ;
      Date_t date = ele.Date() ;
      ASSERT_EQ( mills[i], date ) << "fail to check date mills" ;
   }
}
