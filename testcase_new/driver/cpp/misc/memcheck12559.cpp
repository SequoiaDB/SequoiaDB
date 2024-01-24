/**************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-2270
 *               seqDB-12559:内存泄露验证
 *               use valgrind to check mem leak
 *               manual test, don't run in ci
 * @Modify:      Liang xuewang
 *			 	     2017-12-22
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

#define MAX_NAME_LEN 256

class memcheck12559 : public testBase
{
protected:
   const CHAR* host ;
   const CHAR* svc ;
   const CHAR* user ;
   const CHAR* passwd ;
   const CHAR* rgName ;
   const CHAR* domainName ;
   const CHAR* csName ;
   const CHAR* clName ;
   sdb db ;
   sdbReplicaGroup rg ;
   sdbNode node ;
   sdbDomain domain ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   sdbLob lob ;
   sdbCursor cursor ;

   void SetUp()
   {
      host = ARGS->hostName() ;
      svc = ARGS->svcName() ;
      user = ARGS->user() ;
      passwd = ARGS->passwd() ;
      rgName = "memcheckTestRg12559" ;
      domainName = "memcheckTestDomain12559" ;
      csName = "memcheckTestCs12559" ;
      clName = "memcheckTestCl12559" ;
   }
   void TearDown()
   {
   }
} ;

TEST_F( memcheck12559, sdb )
{
   INT32 rc = SDB_OK ;

   // use db twice to test mem leak
   rc = db.connect( host, svc, user, passwd ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
   rc = db.connect( host, svc, user, passwd ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sbd" ;

   db.disconnect() ;
}

TEST_F( memcheck12559, sdbReplicaGroup ) 
{
   INT32 rc = SDB_OK ;
   rc = db.connect( host, svc, user, passwd ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      db.disconnect() ;
      return ;
   } 

   // use rg twice to test mem leak
   rc = db.createReplicaGroup( rgName, rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create rg" ;
   rc = db.getReplicaGroup( rgName, rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get rg" ;

   rc = db.removeReplicaGroup( rgName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to remove rg" ;
   db.disconnect() ;
}

TEST_F( memcheck12559, sdbNode )
{
   INT32 rc = SDB_OK ;
   rc = db.connect( host, svc, user, passwd ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      db.disconnect() ;
      return ;
   }
   rc = db.createReplicaGroup( rgName, rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create rg" ;

   const CHAR* nodeHost = ARGS->hostName() ;
   const CHAR* nodeSvc = ARGS->rsrvPortBegin() ;
   const CHAR* nodeDir = ARGS->rsrvNodeDir() ;
   CHAR nodePath[ MAX_NAME_LEN ] = { 0 } ;
   sprintf( nodePath, "%s%s%s", nodeDir, "data/", nodeSvc ) ;
   rc = rg.createNode( nodeHost, nodeSvc, nodePath ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create node" ;

   // use node twice to test mem leak
   rc = rg.getNode( nodeHost, nodeSvc, node ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get node" ;
   rc = rg.getNode( nodeHost, nodeSvc, node ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get node" ;

   rc = db.removeReplicaGroup( rgName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to remove rg" ;
   db.disconnect() ;
}

TEST_F( memcheck12559, sdbDomain )
{
   INT32 rc = SDB_OK ;  
   rc = db.connect( host, svc, user, passwd ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      db.disconnect() ;
      return ;
   }
   vector<string> groups ;
   rc = getGroups( db, groups ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   ASSERT_GT( groups.size(), 0 ) << "fail to check group num" ;
   
   // use domain twice to test mem leak
   BSONObj option = BSON( "Groups" << BSON_ARRAY( groups[0].c_str() ) ) ;
   rc = db.createDomain( domainName, option, domain ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create domain" ;
   rc = db.getDomain( domainName, domain ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get domain" ;

   rc = db.dropDomain( domainName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop domain" ;
   db.disconnect() ;
}

TEST_F( memcheck12559, sdbCollectionSpace )
{
   INT32 rc = SDB_OK ;
   rc = db.connect( host, svc, user, passwd ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;

   // use cs twice to test mem leak
   rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
   rc = db.getCollectionSpace( csName, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs" ;

   rc = db.dropCollectionSpace( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
   db.disconnect() ;
}

TEST_F( memcheck12559, sdbCollection )
{
   INT32 rc = SDB_OK ;
   rc = db.connect( host, svc, user, passwd ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
   rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
   
   // use cl twice to check mem leak
   rc = cs.createCollection( clName, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;
   rc = cs.getCollection( clName, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl" ;

   rc = db.dropCollectionSpace( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
   db.disconnect() ;
}

TEST_F( memcheck12559, sdbLob )
{
   INT32 rc = SDB_OK ;
   rc = db.connect( host, svc, user, passwd ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
   rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // use lob twice to test mem leak
   rc = cl.createLob( lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create lob" ;
   const CHAR* writeBuf = "abcdeABCDE" ;
   UINT32 len = strlen( writeBuf ) ;
   rc = lob.write( writeBuf, len ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to write lob" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   OID oid = lob.getOid() ;
   rc = cl.openLob( lob, oid ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to open lob" ;
   CHAR readBuf[ MAX_NAME_LEN ] = { 0 } ;
   UINT32 readLen = 0 ;
   rc = lob.read( len, readBuf, &readLen ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to read lob" ;
   ASSERT_EQ( len, readLen ) << "fail to check readLen" ;
   ASSERT_STREQ( writeBuf, readBuf ) << "fail to check readBuf" ;
   rc = lob.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close lob" ;

   rc = db.dropCollectionSpace( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
   db.disconnect() ;
}

TEST_F( memcheck12559, sdbCursor )
{
   INT32 rc = SDB_OK ;
   rc = db.connect( host, svc, user, passwd ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
   rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   BSONObj doc = BSON( "a" << 1 << "b" << 2 ) ;
   rc = cl.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert" ;

   // use cursor twice to test mem leak
   rc = cl.query( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;
   rc = cl.query( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;

   rc = db.dropCollectionSpace( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
   db.disconnect() ;
}
