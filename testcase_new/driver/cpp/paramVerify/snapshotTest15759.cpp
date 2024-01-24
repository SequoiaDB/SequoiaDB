/**************************************************************************
 * @Description:   test case for C++ driver
 *  
 * @Modify:        wenjing wang Init
 *                 2018-09-26
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"
#include <map>
#include <string>
#include <vector>

using namespace bson ;
class snapshotTest : public testBase
{
protected:
   void SetUp()
   {
      testBase::SetUp() ;
      isStandAlone = FALSE ;
      sdbCursor cursor ;
      INT32 rc = db.getList(cursor, SDB_LIST_GROUPS) ;
      if ( rc == -159 )
      {
         isStandAlone = TRUE ;
         return ;
      } 
      ASSERT_EQ( SDB_OK, rc ) << "Get replica groups list failed, rc = "<< rc ;
      BSONObj ret ;
      while( cursor.next( ret ) != SDB_DMS_EOC )
      {
         BSONElement elem = ret.getFieldDotted("Group") ;
         std::vector<BSONElement> elems = elem.Array() ;
         for ( std::vector<BSONElement>::size_type i = 0; i < elems.size(); ++i)
         {
            BSONObj tmp = elems[i].Obj();
            std::string hostName = tmp.getFieldDotted("HostName").String();
            std::string svcName = tmp.getFieldDotted("Service").Array()[0].Obj().getFieldDotted("Name").String();
            hostName2Svcname.insert( std::pair<std::string,std::string>(hostName, svcName ));
         }
      }
   }
   
   void TearDown()
   {
      testBase::TearDown() ;
   }
protected:
   std::map<std::string, std::string> hostName2Svcname ;
   BOOLEAN isStandAlone ; 
};

TEST_F( snapshotTest, validParameters )
{
   if ( isStandAlone ) return ;
   if ( hostName2Svcname.empty() ) return ;
   
   std::map<std::string, std::string>::const_iterator iter = hostName2Svcname.begin();
   sdbCursor cursor1 ;
   sdbCursor cursor2 ;
   BSONObj cond = BSON( "svcname" << iter->second );
   BSONObj sel = BSON( "NodeName" << "" );
   BSONObj orderBy = BSON( "NodeName" << 1 );
   BSONObj hint = BSON( "$Options" << BSON("mode" << "local") );
   INT32 rc = db.getSnapshot( cursor1, SDB_SNAP_CONFIGS, cond, sel, orderBy, hint ) ;
   ASSERT_EQ( rc, SDB_OK ) << " Get snapshot of node configurations failed, ret=" << rc ;
   
   BSONObj ret ;
   string prevNodeName ;
   while(cursor1.next(ret) != SDB_DMS_EOC )
   {
      BSONElement elem = ret.getFieldDotted("NodeName") ;
      string curNodeName = elem.String() ;
      if ( prevNodeName.empty())
      {
         prevNodeName = curNodeName ;
      }
      else
      {
         ASSERT_GT(curNodeName, prevNodeName) << prevNodeName << ">" << curNodeName ;
         prevNodeName = curNodeName ;
      }
      
      std::string::size_type pos = curNodeName.find(":");
      std::string hostName = curNodeName.substr(0, pos );
      std::map<std::string, std::string>::const_iterator it = hostName2Svcname.find( hostName ) ;
      ASSERT_EQ( it->second, iter->second ) << it->second<< "not equal" << iter->second ;
   }   
}
