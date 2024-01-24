/********************************************************************************
 * @Description: 	test case for C++ driver
 *           	 	seqDB-12510:创建/获取/枚举/修改/删除domain
 *                 seqDB-12658:获取、删除不存在的domain
 * @Modify:      	Liang xuewang Init
 *           		2017-09-11
 ********************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class domainTest12510 : public testBase
{
protected:
   sdbDomain domain ;

   void SetUp()
   {
      testBase::SetUp() ;
   }
   void TearDown()
   {
      testBase::TearDown() ;
   }
} ;

TEST_F( domainTest12510, normalOpr12510 )
{
   INT32 rc = SDB_OK ;
   const CHAR* domainName = "testDomain12510" ;

   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone." << endl ;
      return ;
   }

   // get data groups and create domain
   vector<string> groups ;
   rc = getGroups( db, groups ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get data groups" ; 
   ASSERT_GT( groups.size(), 0 ) ;
   const CHAR* groupName = groups[0].c_str() ;
   BSONObj option = BSON( "Groups" << BSON_ARRAY( groupName ) << "AutoSplit" << true ) ;
   rc = db.createDomain( domainName, option, domain ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create domain " << domainName ;

   // get domain
   sdbDomain domain1 ;
   rc = db.getDomain( domainName, domain1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get domain " << domainName ;

   // get domain name
   ASSERT_STREQ( domainName, domain.getName() ) << "fail to check domain name" ;

   // list domains
   sdbCursor cursor ;
   BSONObj cond = BSON( "Name" << domainName ) ;
   rc = db.listDomains( cursor, cond, _sdbStaticObject, _sdbStaticObject, _sdbStaticObject ) ;  // error
   ASSERT_EQ( SDB_OK, rc ) << "fail to list domains" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next in cursor" ;
   ASSERT_STREQ( domainName, obj.getField( "Name" ).String().c_str() ) 
                 << "fail to check list domain name" ;
   vector<BSONElement> domainGroups = obj.getField( "Groups" ).Array() ;
   ASSERT_EQ( 1, domainGroups.size() ) << "fail to check domain groups num" ;
   ASSERT_STREQ( groupName, domainGroups[0].Obj().getField( "GroupName" ).String().c_str() ) 
                 << "fail to check domain group" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // alter domain
   if( groups.size() >= 2 )
   {
      const CHAR* groupName1 = groups[1].c_str() ;
      cout << "domain group: " << groupName << " " << groupName1 << endl ;
      option = BSON( "Groups" << BSON_ARRAY( groupName << groupName1 ) ) ;
      rc = domain.alterDomain( option ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to alter domain " << domainName ;
      rc = db.listDomains( cursor, cond, _sdbStaticObject, _sdbStaticObject, _sdbStaticObject ) ;  // error
      ASSERT_EQ( SDB_OK, rc ) << "fail to list domains" ;
      rc = cursor.next( obj ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to get next in cursor" ;
      vector<BSONElement> domainGroups = obj.getField( "Groups" ).Array() ;
      ASSERT_EQ( 2, domainGroups.size() ) << "fail to check domain groups num" ;
      ASSERT_STREQ( groupName, domainGroups[0].Obj().getField( "GroupName" ).String().c_str() ) 
                    << "fail to check domain group 1" ;
      ASSERT_STREQ( groupName1, domainGroups[1].Obj().getField( "GroupName" ).String().c_str() ) 
                    << "fail to check domain group 2" ;
      rc = cursor.close() ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
   }

	// drop domain
	rc = db.dropDomain( domainName ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to drop domain " << domainName ;
   rc = db.getDomain( domainName, domain ) ;
   ASSERT_EQ( SDB_CAT_DOMAIN_NOT_EXIST, rc ) << "fail to check drop domain " << domainName ;
}

TEST_F( domainTest12510, notExist12568 )
{
   INT32 rc = SDB_OK ;
   const CHAR* domainName = "notExistDomain12658" ;

   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone." << endl ;
      return ;
   }

   // get not exist domain
   rc = db.getDomain( domainName, domain ) ;
   ASSERT_EQ( SDB_CAT_DOMAIN_NOT_EXIST, rc )
              << "fail to test get not exist domain " << domainName ;

   // drop not exist domain
   rc = db.dropDomain( domainName ) ;
   ASSERT_EQ( SDB_CAT_DOMAIN_NOT_EXIST, rc )
              << "fail to test drop not exist domain " << domainName ;
}
