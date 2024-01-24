/**************************************************************
 * @Description: 	test case for C++ driver
 *           	 	seqDB-12657:枚举domain中的cs、cl
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

class domainTest12657 : public testBase
{
protected:
   sdbDomain domain ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   const CHAR* domainName ;
   const CHAR* csName ;
   const CHAR* clName ;
   const CHAR* clFullName ;

   void SetUp()
   {
      testBase::SetUp() ;
      domainName = "testDomain12657" ;
      csName = "domainTestCs12657" ;
      clName = "domainTestCl12657" ;
      clFullName = "domainTestCs12657.domainTestCl12657" ;
   }
   void TearDown()
   {
      testBase::TearDown() ;
   }
} ;

TEST_F( domainTest12657, listCsCl12657 )
{
   INT32 rc = SDB_OK ;
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
   rc = db.getDomain( domainName, domain ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get domain " << domainName ;

   // create cs cl in domain
   option = BSON( "Domain" << domainName ) ;
   rc = db.createCollectionSpace( csName, option, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
   rc = cs.createCollection( clName, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;
   
   // list cs in domain
   sdbCursor cursor ;
   rc = domain.listCollectionSpacesInDomain( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to list cs in domain" ;
   BSONObj obj ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_STREQ( csName, obj.getField( "Name" ).String().c_str() ) << "fail to check list cs in domain" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // list cl in domain
   rc = domain.listCollectionsInDomain( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to list cl in domain" ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get next" ;
   ASSERT_STREQ( clFullName, obj.getField( "Name" ).String().c_str() ) << "fail to check list cl in domain" ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   // drop cs
   rc = db.dropCollectionSpace( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;

	// drop domain
	rc = db.dropDomain( domainName ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to drop domain " << domainName ;
}
