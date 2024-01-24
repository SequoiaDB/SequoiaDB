/*******************************************************************************
*@Description : Test domain C++ driver, include createDomain/sdbDropDomain/
*               /sdbGetDomain/sdbListDomains/sdbAlterDomain/
*               /sdbListCollectionSpacesInDomain/sdbListCollectionsInDomain
*@Modify List :
*               2014-7-15   xiaojun Hu   Change [adb abnormal test]
*******************************************************************************/

#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

#define SDB_DOMAIN_NOTEXIST         -214
#define SDB_INVALIDARG              -6

using namespace std ;
using namespace sdbclient ;

TEST( domainTest, not_connect )
{
   sdbDomain dm ;
   BSONObj obj ;
   INT32 rc = SDB_OK ;

   rc = dm.alterDomain( obj ) ;
   ASSERT_EQ( SDB_NOT_CONNECTED, rc ) ;
}

// Testing abnormal in Domain
TEST( domainTest, abnormal )
{
   sdb db ;
   sdbDomain dm, dm1, dm2 ;
   //sdbCollectionSpace cs ;
   //sdbCollection cl ;
   sdbCursor cursor ;
   //const CHAR *pCS1 = "" ;
   const CHAR *gtDomName   = "theDomainNamegreatethan127bytebbbbbbbbbbbbbbbbbbb"
                             "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
                             "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
                             "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
                             "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
                             "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
                             "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb" ;
   const CHAR *domName     = "rightDomainName" ;
   const CHAR *notExistDom = "NotExistDomainName" ;
   INT32 rc                = SDB_OK ;

   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   // Build domain option object
   BSONObjBuilder autoObj ;
   autoObj.append( "AutoSplit",true) ;
   BSONObj options = autoObj.obj() ;
   CHAR gtdomname[512] ;
   CHAR domname[64] ;
   CHAR notexistdom[64] ;
   // Deal with domain name
   getUniqueName(  gtDomName, gtdomname, 512 ) ;
   getUniqueName( domName, domname, 64 ) ;
   getUniqueName( notExistDom, notexistdom, 64 ) ;

   // Connect to Sdb
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( rc, SDB_OK ) << "Failed to connect to SDB, rc = " << rc ;

   // Run mode is standalone
   rc = db.getList( cursor, SDB_LIST_GROUPS ) ;
   if ( SDB_RTN_COORD_ONLY == rc )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }
   ASSERT_EQ( SDB_OK, rc ) << "Failed to getList 'SDB_LIST_GROUPS', "
                                "rc = " << rc ;

   // Create domain
   rc = db.dropDomain( domname ) ;
   ASSERT_TRUE( SDB_OK == rc || SDB_DOMAIN_NOTEXIST )
                << "Failed to drop Domain, rc = " << rc  ;
   rc = db.createDomain( domname, options, dm ) ;
   ASSERT_EQ( rc, SDB_OK )
                << "Failed to create right domName, rc = " << rc ;
   // Domain greater than 127
   rc = db.createDomain( gtdomname, options, dm ) ;
   EXPECT_TRUE( SDB_INVALIDARG == rc )
                << "Failed to excute create Domaint greater than, rc = " << rc ;

   // Domain name is NULL
   rc = db.createDomain( NULL, options, dm ) ;
   EXPECT_TRUE( SDB_INVALIDARG == rc )
                << "Failed to excute create NULL domName, rc = " << rc  ;
   // Using sdbDomainHandle dm in getDomain
   //cout << "domain cursor : " << dm << endl ;
   rc = db.getDomain( domname, dm ) ;
   EXPECT_TRUE( SDB_OK == rc )
                << "Failed to excute getDomain, rc = " << rc ;
   // Get Not Exist domain
   rc = db.getDomain( notexistdom, dm2 ) ;
   EXPECT_TRUE( SDB_DOMAIN_NOTEXIST == rc )
                << "Failed to excute get not exist domain, rc = " << rc ;
   rc = db.getDomain( domname, dm2 ) ;
   EXPECT_TRUE( SDB_OK == rc ) << "Failed to get right domain, rc = " << rc ;

//   rc = db.listDomains( cursor, 0, 0, 0, 0 ) ;
//   ASSERT_EQ( rc, SDB_OK ) << "Failed to list Domains, rc = " << rc ;

   // Alter domain by sepcify option is NULL
//   rc = dm2.alterDomain( NULL ) ;
//   ASSERT_EQ( rc, SDB_OK ) << "Failed to alterDomain NULL, rc = " << rc ;

   rc = dm2.listCollectionSpacesInDomain( cursor ) ;
   EXPECT_TRUE( SDB_OK == rc ) << "Failed to list Collection Space, rc = " << rc ;

   rc = dm2.listCollectionsInDomain( cursor ) ;
   EXPECT_TRUE( SDB_OK == rc ) << "Failed to list Collection, rc = " << rc ;

   rc = db.dropDomain( domname ) ;
   ASSERT_EQ( rc, SDB_OK ) << "Failed to drop Domain, rc = " << rc ;

   db.disconnect() ;
}

// Testing normal in domain
TEST(domainTest, normalAll )
{
   sdb db ;
   sdbDomain dm ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbCursor cursor, cur1 ;
   const CHAR *pCS1 = "cs_domain" ;
   const CHAR *pCL1 = "cl_domain" ;
   const CHAR *pDM1 = "dm_cpp" ;
   // initialize local variables
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   const CHAR *pGroupName                   = GROUPNAME ;
   INT32 rc                                 = SDB_OK ;
   BSONObj obj ;
   CHAR pCS[64] ;
   CHAR pCL[64] ;
   CHAR pDM[64] ;
   // Deal with name
   getUniqueName( pCS1, pCS, 64 ) ;
   getUniqueName( pCL1, pCL, 64 ) ;
   getUniqueName( pDM1, pDM, 64 ) ;
   // initialize the work environment
   //rc = initEnv() ;
   //ASSERT_EQ( SDB_OK, rc ) ;

   // connect to database
   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   ASSERT_EQ( SDB_OK, rc ) << "Failed to connect to SDB, rc = " << rc ;
   // is in standalone
   rc = db.getList( cursor, SDB_LIST_GROUPS ) ;
   if ( SDB_RTN_COORD_ONLY == rc )
   {
      cout << "Run mode is standalone." << endl ;
      return ;
   }
   ASSERT_EQ( SDB_OK, rc ) << "Failed to getList 'SDB_LIST_GROUPS', "
                                "rc = " << rc ;

   db.dropCollectionSpace( pCS ) ;
   // drop domain
   rc = db.dropDomain( pDM ) ;
   // create domain
   BSONObjBuilder bob ;
   bob.append( "AutoSplit", true ) ;
   BSONObj options = bob.obj() ;
   cout << "Options : " << options << endl ;
   rc = db.createDomain( pDM, options, dm ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = dm.listCollectionSpacesInDomain( cur1 ) ;
   ASSERT_EQ( rc, SDB_OK ) ;

   sdbDomain dm2 ;
   rc = db.getDomain( pDM, dm2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // alter domain
   BSONObj arr2 = BSON( "0" << pGroupName ) ;
   BSONObjBuilder bob2 ;
   bob2.appendArray( "Groups", arr2 ) ;
   BSONObj options2 = bob2.obj() ;
   rc = dm2.alterDomain( options2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   // list cs in domain
   BSONObj cs_opt = BSON( "PageSize" << 4096 << "Domain" << pDM ) ;
   rc = db.createCollectionSpace( pCS, cs_opt, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   //sdbCursor cur1 ;
   rc = dm2.listCollectionSpacesInDomain( cur1 ) ;
   cout << "cursor1 has record: " << endl ;
   ASSERT_EQ( SDB_OK, rc ) ;
   displayRecord( cur1 ) ;
   // list cl in domain
   BSONObj options3 = BSON( "ShardingKey" << BSON("a"<<1)
                            << "ShardingType" << "hash" ) ;
   rc = cs.createCollection( pCL, options3, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   sdbCursor cur2 ;
   rc = dm2.listCollectionsInDomain( cur2 ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << "cursor2 has record: " << endl ;
   displayRecord( cur2 ) ;
   // drop collectionspace
   db.dropCollectionSpace( pCS ) ;
   // drop domain
   rc = db.dropDomain( pDM ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   db.disconnect() ;
}



// TODO:
/*
getName

*/
