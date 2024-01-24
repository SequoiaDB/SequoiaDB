/*******************************************************************************
 * @Description:    test case for C++ driver
 *                  
 *                  
 * @Modify:         wenjing wang Init
 *                  2019-10-09
 *******************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"
#include <set>
#include <string>

using namespace sdbclient ;
using namespace bson ;

class listTest : public testBase
{
protected:   
   int dataGroupNum ;

   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      
      dataGroupNum = 0 ;

      BSONObj cond = BSON( "GroupID" << BSON( "$gte" << 1000 ) ) ;
      sdbCursor cursor ;
      rc = db.getList( cursor, SDB_LIST_GROUPS, cond ) ;
      if ( rc == -159){
         return ;
      }
     
      BSONObj obj ;
      while ( cursor.next(obj) == 0 )
      {
         dataGroupNum++ ;
      }
   }
   
   void TearDown()
   {
      testBase::TearDown() ;
   }
} ;

TEST_F( listTest,  AllParameters)
{
   INT32 rc = SDB_OK ;
   
   int prevGroupId = 0 ;
   BSONObj cond = BSON( "GroupID" << BSON( "$gt" << 2 ) ) ;
   BSONObj sel = BSON( "GroupName" << "" << "GroupID" << "" ) ;
   BSONObj hint = BSON( "GroupID" << 1 ) ;
   BSONObj orderby = BSON( "GroupID" << 1 ) ;
   
   sdbCursor cursor ;
   rc = db.getList( cursor, SDB_LIST_GROUPS, cond, sel, orderby, hint, 0, 2 ) ;
   if ( rc == -159){
      return ;
   }
   
   ASSERT_EQ( SDB_OK, rc ) << "fail to getList" ;
   
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   
   ASSERT_EQ(obj.hasField("GroupID"), 1);
   ASSERT_EQ(obj.hasField("GroupName"), 1);
   prevGroupId = obj.getField("GroupID").Int();

   std::set<std::string> fields ;
   obj.getFieldNames( fields) ;
   ASSERT_EQ(2, fields.size()) ;

   if ( dataGroupNum >= 2 ){
      rc = cursor.next( obj ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;

      ASSERT_EQ(obj.hasField("GroupID"), 1);
      ASSERT_EQ(obj.hasField("GroupName"), 1);

      int curGroupId = obj.getField("GroupID").Int(); 
      ASSERT_GT(curGroupId, prevGroupId) ;
   
      obj.getFieldNames( fields) ;
      ASSERT_EQ(2, fields.size()) ;
   }

   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_DMS_EOC, rc ) << "fail to get next" ;
   cursor.close() ;
}
