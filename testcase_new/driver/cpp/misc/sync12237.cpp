/**************************************************************
 * @Description: test case for c++ driver
 *				     seqDB-12237:测试syncDB接口
 * @Modify     : Liang xuewang Init
 *			 	     2017-09-22
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

class syncTest12237 : public testBase
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

TEST_F( syncTest12237, legalOption12237 )
{
	INT32 rc = SDB_OK ;

   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

	// get data groups
	vector<string> groups ;
	rc = getGroups( db, groups ) ;
	ASSERT_EQ( SDB_OK, rc ) ;

	// get group name and group id
	const CHAR* rgName = groups[0].c_str() ;
	sdbReplicaGroup rg ;
	rc = db.getReplicaGroup( rgName, rg ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to get rg " << rgName ;
	BSONObj detail ;
	rc = rg.getDetail( detail ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to get rg detail" ;
	INT32 groupId = detail.getField( "GroupID" ).Int() ;
	cout << "group: name = " << rgName << ", id = " << groupId << endl ;
		
	// get slave data node 
   sdbNode node ;
	rc = rg.getSlave( node ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get slave node" ;
	const CHAR* hostName = node.getHostName() ;
   const CHAR* svcName = node.getServiceName() ;
   const CHAR* nodeName = node.getNodeName() ;
	cout << "node: hostname = " << hostName << ", svcName = " << svcName 
		  << ", nodeName = " << nodeName << endl ;

	// create cs cl in group
	const CHAR* csName = "syncTestCs12237" ;
	sdbCollectionSpace cs ;
	rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
	const CHAR* clName = "syncTestCl12237" ;
	sdbCollection cl ;
	BSONObj option = BSON( "Group" << rgName << "ReplSize" <<0 ) ;
	rc = cs.createCollection( clName, option, cl ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;

	// sync with no option
	rc = db.syncDB() ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to sync with no option" ;

	// sync with option
	BSONObj syncOption = BSON( "Deep" << 1 << "Block" << false << "CollectionSpace" << csName << 
                              "Global" << false << "GroupId" << groupId << "GroupName" << rgName << 
                              "HostName" << hostName << "svcname" << svcName ) ;
	rc = db.syncDB( option ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to sync with option" ;
	
	// drop cs
	rc = db.dropCollectionSpace( csName ) ;
	ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
}

TEST_F( syncTest12237, illegalOption12237 )
{
	INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

	// sync with invalid option
	BSONObj option = BSON( "HostName" << "InvalidHost" ) ;
	rc = db.syncDB( option ) ;
	ASSERT_EQ( SDB_CLS_NODE_NOT_EXIST, rc ) << "fail to test sync with illegal option" ;
}
