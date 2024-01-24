/**************************************************************
 * @Description: get all kind of Operator of monitor
 *               seqDB-28014 : get all kind of Op
 * @Modify     : Tao Tang
 *               2022-09-27
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include <vector>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;


INT32 testcaseID = 28014;
const char *userNameAdmin = "admin_28014" ;
const char *userNameMonitor = "monitor_28014" ;
const char *csName = "monitor_testCS" ;
const char *clName = "monitor_testCL" ;
BSONObj optionAdmin = BSON( "Role" << "admin" ) ;
BSONObj optionMonitor = BSON( "Role" << "monitor" ) ;

class monitorAuth : public testBase 
{
protected:
   sdbCursor cursor ;
   BSONObj res ;

   void SetUp()
   {
      testBase::SetUp() ;

      db.createUsr( userNameAdmin , userNameAdmin , optionAdmin ) ;
      db.createUsr( userNameMonitor , userNameMonitor , optionMonitor ) ;

      sdbCollectionSpace cs ;
      sdbCollection cl ;
      db.createCollectionSpace( csName , SDB_PAGESIZE_4K , cs ) ;
      cs.createCollection( clName , cl ) ;
      cl.insert( BSON( "age" << 18 ) ) ;

   }

   void TearDown()
   {
      db.removeUsr( userNameMonitor , userNameMonitor ) ;
      db.removeUsr( userNameAdmin , userNameAdmin ) ;
      db.dropCollectionSpace( csName ) ;

      testBase::TearDown() ;
   }
} ;

TEST_F( monitorAuth , authOp )
{
   INT32 rc = SDB_OK ;
   sdb coorddb ;

   SINT64 count ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   coorddb.connect( ARGS->hostName() , ARGS->svcName() ,
                    userNameMonitor , userNameMonitor ) ;

   // a
   rc = coorddb.getSnapshot( cursor , SDB_SNAP_COLLECTIONS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = coorddb.getList( cursor , SDB_LIST_COLLECTIONS ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // b
   rc = coorddb.listCollections( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // c
   rc = coorddb.getCollectionSpace( csName, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = coorddb.setSessionAttr( BSON( "Timeout" << 10000 ) ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = coorddb.getSessionAttr( res ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // d
   rc = cs.listCollections( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // e
   rc = cs.getCollection( clName , cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.getDetail( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cl.getCount( count ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // f
   rc = cl.getCount( count , BSON("age" << BSON("$gt" << 10 ) ) ) ;
   ASSERT_EQ( SDB_OK , rc ) ;

   // g
   sdbReplicaGroup rg ;
   sdbNode catalog ;
   sdb catadb ;
   rc = coorddb.getReplicaGroup( "SYSCatalogGroup", rg ) ;
   ASSERT_EQ( SDB_OK , rc ) ;
   rc = rg.getMaster( catalog ) ;
   ASSERT_EQ( SDB_OK , rc ) ;


   // ddl
   const char *tmpcsName = "monitor_testCS_tmp" ;
   const char *tmpclName = "monitor_testCL_tmp" ;

   rc = coorddb.createCollectionSpace( tmpcsName , SDB_PAGESIZE_4K , cs ) ;
   ASSERT_EQ( SDB_NO_PRIVILEGES , rc ) ;
   rc = coorddb.getCollectionSpace( csName , cs ) ;
   rc = cs.createCollection( tmpclName , cl ) ;
   ASSERT_EQ( SDB_NO_PRIVILEGES , rc ) ;

   // dml
   rc = cs.getCollection( clName , cl ) ;
   rc = cl.insert( BSON( "num" << 1001 ) ) ;
   ASSERT_EQ( SDB_NO_PRIVILEGES , rc ) ;

   // dql
   rc = cl.query( cursor ) ;
   ASSERT_EQ( SDB_NO_PRIVILEGES , rc ) ;

}
