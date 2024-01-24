/*******************************************************************************
 * @Description:    test case for C++ driver
 *        seqDB-19953 : snapshot支持过滤、选择、排序、skip、limit           
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

class snapShotTest19953 : public testBase
{
protected:   
   int replNodeNum ;

   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      
      replNodeNum = 0 ;

      BSONObj cond = BSON( "$or" << BSON_ARRAY( BSON("GroupID" << 1)<<BSON( "GroupID"<<BSON( "$gte" << 1000 ) ) ) ) ;
      std::cout << cond.toString() << std::endl ;
      sdbCursor cursor ;
      rc = db.getList( cursor, SDB_LIST_GROUPS, cond ) ;
      if ( rc == -159){
	 replNodeNum = 1 ;
         return ;
      }
     
      BSONObj obj ;
      while ( cursor.next(obj) == 0 )
      {
         vector<BSONElement> group = obj.getField("Group").Array() ;
         replNodeNum += group.size() ;
      }
      cursor.close() ;
   }
   
   void TearDown()
   {
      testBase::TearDown() ;
   }
} ;

TEST_F( snapShotTest19953,  AllParameters)
{
   INT32 rc = SDB_OK ;
   int curReplNodeNum = 0 ;
   BSONObj cond = BSON( "Contexts.Type" << "DUMP" ) ;
   BSONObj sel = BSON( "NodeName" << "" << "SessionID" << "" ) ;
   BSONObj hint = BSON( "NodeName" << 1 ) ;
   BSONObj orderby = BSON( "NodeName" << 1 ) ;
   
   std::cout << SDB_SNAP_CONTEXTS_CURRENT << cond.toString() << std::endl; 
   sdbCursor cursor ;
   rc = db.getSnapshot( cursor, SDB_SNAP_CONTEXTS) ;
   BSONObj obj ;
   while ( cursor.next(obj) == 0) ;
   cursor.close();

   rc = db.getSnapshot( cursor, SDB_SNAP_CONTEXTS_CURRENT, cond, sel, orderby, hint) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getList" ;
   
   std::vector<BSONObj> resObjs ;
   while ((rc = cursor.next(obj)) == 0){
      curReplNodeNum++ ;
      if ( curReplNodeNum >=3 && curReplNodeNum < 6 ){
         resObjs.push_back(obj) ;
      }
   }
   cursor.close() ;
   
   ASSERT_EQ( curReplNodeNum, replNodeNum ) << "fail to getSnapshot"  ;
   if ( resObjs.empty() ){
      return ;
   }
   
   rc = db.getSnapshot( cursor, SDB_SNAP_CONTEXTS_CURRENT, cond, sel, orderby, hint, 3, 3 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to getList" ;
   
   int pos = 0 ;
   while (cursor.next(obj) == 0){
      //ASSERT_EQ( obj.getField("NodeName").String(), resObjs[pos].getField("NodeName").String()) << obj.toString() << " !=" << resObjs[pos].toString();
      //ASSERT_EQ( obj.getField("SessionID").Int(), resObjs[pos].getField("SessionID").Int()) << obj.toString() << " !=" << resObjs[pos].toString();
      pos++ ;
   }
   cursor.close() ;
   
   ASSERT_EQ( pos, resObjs.size() ) << "fail to getSnapshot" ;
}

