/**************************************************************
 * @Description: 	test case for C++ driver
 *           	 	seqDB-14685:枚举domain中的rg
 * @Modify:      	Liang xuewang Init
 *           		2017-09-12
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class listGroupInDomainTest : public testBase
{
protected:
   sdbDomain domain ;
   const CHAR* domainName ;

   void SetUp()
   {
      testBase::SetUp() ;
      domainName = "testDomain14685" ;
   }
   void TearDown()
   {
      testBase::TearDown() ;
   }
} ;

TEST_F( listGroupInDomainTest, listGroupInDomain )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }
   
   INT32 rc = SDB_OK ;
   vector<string> groups ;
   rc = getGroups( db, groups ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   if( groups.size() == 0 )
   {
      cout << "No data groups" << endl ;
      return ;
   }

   const CHAR* groupName = groups[0].c_str() ;
   BSONObj option = BSON( "Groups" << BSON_ARRAY( groupName ) ) ;
   rc = db.createDomain( domainName, option, domain ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create domain" ;

   rc = db.getDomain( domainName, domain ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get domain" ;

   sdbCursor cursor ;
   rc = domain.listReplicaGroupInDomain( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to list groups in domain" ;
   
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to next" ;
   ASSERT_EQ( domainName, obj.getField( "Name" ).String() ) << "fail to check domainName" ;
   vector<BSONElement> arr = obj.getField( "Groups" ).Array() ;
   ASSERT_EQ( 1, arr.size() ) << "fail to check groups num" ;
   ASSERT_EQ( groupName, arr[0].Obj().getField( "GroupName" ).String() ) << "fail to check group" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   rc = db.dropDomain( domainName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop domain" ;
}
