/**************************************************************
 * @Description: create/list/remove procedure 
 *               seqDB-12522 : create/list/remove procedure
 * @Modify     : Suqiang Ling
 *               2017-09-04
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

class procedure12522 : public testBase {} ;

TEST_F( procedure12522, testProcedure )
{
   if( isStandalone( db ) )
   {
      cout << "skip this test for standalone" << endl ; 
      return ;
   }

   // create procedure
   INT32 rc = SDB_OK ;
   string funcName = "sum12522" ;
   string funcCode = "function " + funcName + "( x, y ) { return x + y; }" ;
   rc = db.crtJSProcedure( funcCode.c_str() ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create procedure" ;

   // list procedure
   sdbCursor listCursor ;
   BSONObj cond = BSON( "name" << funcName.c_str() ) ;
   rc = db.listProcedures( listCursor, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to list procedure" ;

   // check js code
   BSONObj listRes ;
   rc = listCursor.next( listRes ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get listCursor next" ;
   string actCode = listRes.getField( "func" ).valuestr() ;
   ASSERT_STREQ( funcCode.c_str(), actCode.c_str() ) << "procedure funcCode is wrong" ;
   rc = listCursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close listCursor" ;

   // call procedure
   sdbCursor evalCursor ;
   SDB_SPD_RES_TYPE type = SDB_SPD_RES_TYPE_NUMBER ;
   BSONObj errmsg ;
   string evalCode = funcName + "( 1, 2 )" ;
   rc = db.evalJS( evalCode.c_str(), type, evalCursor, errmsg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to call procedure, errmsg: " << errmsg ; 

   // check return
   BSONObj evalRes ;
   rc = evalCursor.next( evalRes ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get evalCursor next" ;
   int expReturn = 3 ;
   int actReturn = evalRes.getField( "value" ).Int() ; 
   ASSERT_EQ( expReturn, actReturn ) << "result that procedure returns is wrong" ;
   rc = evalCursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to close evalCursor" ;
   ASSERT_EQ( SDB_SPD_RES_TYPE_NUMBER, type ) << "precedure return type is wrong" ;
   
   // remove procedure
   rc = db.rmProcedure( funcName.c_str() ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to remove procedure" ; // TODO: check procedure was deleted!
   rc = db.evalJS( evalCode.c_str(), type, evalCursor, errmsg ) ;
   ASSERT_EQ( SDB_SPT_EVAL_FAIL, rc ) << "call procedure shouldn't succeed after deletion" ;
}
