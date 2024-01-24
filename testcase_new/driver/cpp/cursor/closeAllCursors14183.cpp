/**************************************************************
 * @Description: test case for Jira questionaire
 *               seqDB-14813:执行closeAllCursor关闭游标
 *               SEQUOIADBMAINSTREAM-2220
 * @Modify     : Liang xuewang Init
 *               2018-03-21
 ***************************************************************/
#include <iostream>
#include <gtest/gtest.h>
#include <client.hpp>
#include <sdbConnectionPool.hpp>
#include <sdbConnectionPoolComm.hpp>
#include <stdlib.h>
#include "testcommon.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace std ;
using namespace bson ;

class closeAllCursorsTest14183 : public testing::Test
{
protected:
   const CHAR* host ;
   const CHAR* svc ;
   const CHAR* user ;
   const CHAR* passwd ;
   string url ;
   const CHAR* csname ;
   const CHAR* clname ;
   sdb db ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   
   void SetUp()
   {
      host = ARGS->hostName() ;
      svc = ARGS->svcName() ;
      user = ARGS->user() ;
      passwd = ARGS->passwd() ;
      url = ARGS->coordUrl() ;
      csname = "closeAllCursorsTestCs14183" ;
      clname = "closeAllCursorsTestCl14183" ;
   }
   void TearDown()
   {
   }
} ;

INT32 createCSCL( sdb& db, sdbCollectionSpace& cs, sdbCollection& cl, const char* csname, const char* clname )
{
   INT32 rc = SDB_OK ;

   rc = db.createCollectionSpace( csname, SDB_PAGESIZE_4K, cs ) ;
   CHECK_RC( SDB_OK, rc, "fail to create cs" ) ;
   rc = cs.createCollection( clname, cl ) ;
   CHECK_RC( SDB_OK, rc, "fail to create cl" ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 insertAndQuery( sdbCollection& cl, sdbCursor& cursor )
{
   INT32 rc = SDB_OK ;

   BSONObj obj = BSON( "a" << "1" ) ;
   rc = cl.insert( obj ) ;
   CHECK_RC( SDB_OK, rc, "fail to insert" ) ;
   rc = cl.query( cursor, obj ) ;
   CHECK_RC( SDB_OK, rc, "fail to query" ) ;

done:
   return rc ;
error:
   goto done ;
}

// test close all cursors with normal connection
TEST_F( closeAllCursorsTest14183, normalConn )
{
   INT32 rc = SDB_OK ;

   rc = db.connect( host, svc, user, passwd ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb" ;
   rc = createCSCL( db, cs, cl, csname, clname ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbCursor cursor ;
   rc = insertAndQuery( cl, cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = db.closeAllCursors() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to closeAllCursors" ;

   // check cursor
   BSONObj res ;
   rc = cursor.next( res ) ;
   ASSERT_EQ( SDB_DMS_CONTEXT_IS_CLOSE, rc ) << "fail to check cursor" ;

   rc = db.dropCollectionSpace( csname ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
   db.disconnect() ;
}

// test close all cursors with connectionpool connection
TEST_F( closeAllCursorsTest14183, connectionPoolConn )
{
   INT32 rc = SDB_OK ;

   sdbConnectionPool ds ;
   sdbConnectionPoolConf conf ;

   conf.setAuthInfo( user, passwd ) ;
   rc = ds.init( url, conf ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to init connectionpool" ;

   sdb* conn = &db ;
   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection" ;
   rc = createCSCL( *conn, cs, cl, csname, clname ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   sdbCursor cursor ;
   rc = insertAndQuery( cl, cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ds.releaseConnection( conn ) ;

   BSONObj res ;
   rc = cursor.next( res ) ;
   ASSERT_EQ( rc, SDB_DMS_CONTEXT_IS_CLOSE ) << "fail to check cursor" ;

   rc = ds.getConnection( conn ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get connection again" ;
   rc = (*conn).dropCollectionSpace( csname ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;

   ds.close() ;
}
