/*****************************************************************************
*@Description : SequoiaDB CPP driver testcase : Query()
*@Modify List :
*               2014-10-27   xiaojun  Hu   Init
*****************************************************************************/
#include <stdio.h>
#include <gtest/gtest.h>
#include "client.hpp"
#include "testcommon.hpp"
#include <string>
#include <iostream>

using namespace std ;
using namespace sdbclient ;
/*****************************************************************************
*@Description : Query one record, specify argument : numToReturn = 0 [limit].
*@Modify List :
*               2014-10-27   xiaojun  Hu   Init
*****************************************************************************/

INT32 createProcedure( sdb &db, const CHAR *code )
{
   return db.crtJSProcedure( code ) ;
}

TEST( procedure, eval )
{
   INT32 rc = SDB_OK ;
   const CHAR *csName = "sdb_clientcpp_collection_test" ;
   const CHAR *clName = "sdb_query_limit_one" ;
   const CHAR *pHostName                    = HOST ;
   const CHAR *pPort                        = SERVER ;
   const CHAR *pUsr                         = USER ;
   const CHAR *pPasswd                      = PASSWD ;
   sdb db ;
   sdbCursor cursor ;
   sdbCursor rgCursor ;
   SDB_SPD_RES_TYPE type = SDB_SPD_RES_TYPE_VOID ;
   BSONObj obj ;
   BSONObj err ;
   BOOLEAN isStandalone = false ;

   rc = db.connect( pHostName, pPort, pUsr, pPasswd ) ;
   EXPECT_EQ( SDB_OK, rc ) ;

   // Inspect the run mode
   rc = db.listReplicaGroups( rgCursor ) ;
   if ( -159 == rc )
   {
      // when standalone
      isStandalone = true ;
      goto done ;
   }
   ASSERT_EQ( SDB_OK, rc ) ;

   db.rmProcedure("a1") ;
   db.rmProcedure("a2") ;

   rc = createProcedure( db, "function a1(){return 1;}" ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = createProcedure( db, "function a2(){return 'abc';}" ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   rc = db.evalJS( "a1()", type, cursor, err ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_EQ( SDB_SPD_RES_TYPE_NUMBER, type ) ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << obj.toString( FALSE, TRUE ) << endl ;

   rc = db.evalJS( "a2()", type, cursor, err ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   ASSERT_EQ( SDB_SPD_RES_TYPE_STR, type ) ;
   rc = cursor.next( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   cout << obj.toString( FALSE, TRUE ) << endl ;
   cursor.close() ;
done:
   if ( true == isStandalone )
   {
      cout << "===>" << endl ;
      cout << "Run mode is standalone" << endl ;
      cout << "<===" << endl ;
   }
}

