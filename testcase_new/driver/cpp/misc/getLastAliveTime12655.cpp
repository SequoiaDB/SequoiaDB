/**************************************************************
 * @Description: test getLastAliveTime()
 *               seqDB-12655 : test getLastAliveTime()
 * @Modify     : Suqiang Ling
 *               2017-09-11
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <stdio.h>
#include <string>
#include <iostream>
#include <time.h>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace std;
using namespace sdbclient;
using namespace bson; 

class getLastAliveTime12655 : public testBase 
{
protected:
   time_t endtime ;
   time_t timediff ;

   INT32 checkLastAliveTime()
   {
      ossSleep( 2050 ) ;
      time( &endtime ) ;
      timediff = difftime( endtime, db.getLastAliveTime() ) ;
      INT32 rc = SDB_OK ;
      if( timediff != 2 && timediff != 3 ) //can be more than 2s if machine seize up
      {
         cout << "timediff: " << timediff << endl ;
         rc = SDB_TEST_ERROR ;
      }
      return rc ;
   }

} ;

TEST_F( getLastAliveTime12655, getLastAliveTime )
{   
   INT32 rc = SDB_OK ;
   time_t endtime ;
   time_t timediff ;
   
   rc = checkLastAliveTime() ; 
   ASSERT_EQ( SDB_OK, rc ) << "wrong lastAliveTime" ;

   // create cs
   const CHAR *pCsName = "getTime12655" ;
   sdbCollectionSpace cs ;
   rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) ; 
   
   rc = checkLastAliveTime() ; 
   ASSERT_EQ( SDB_OK, rc ) << "wrong lastAliveTime" ;
   
   // create cl
   const CHAR *pClName = "getTime12655" ;
   sdbCollection cl ; 
   rc = cs.createCollection( pClName, cl ) ; 
   ASSERT_EQ( SDB_OK, rc ) ; 
   
   rc = checkLastAliveTime() ; 
   ASSERT_EQ( SDB_OK, rc ) << "wrong lastAliveTime" ;
   
   // create cl again, throw error   
   rc = cs.createCollection( pClName, cl ) ; 
   ASSERT_EQ( SDB_DMS_EXIST, rc ) ; 
   
   rc = checkLastAliveTime() ; 
   ASSERT_EQ( SDB_OK, rc ) << "wrong lastAliveTime" ;
   
   // insert
   rc = cs.getCollection( pClName, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   BSONObj obj ;
   obj = BSON ( "name" << "tom" << "age" << 24 ) ;
   rc = cl.insert( obj ) ;
   ASSERT_EQ( SDB_OK, rc ) ; 
   
   rc = checkLastAliveTime() ; 
   ASSERT_EQ( SDB_OK, rc ) << "wrong lastAliveTime" ;
   
   // query 
   sdbCursor cursor ;
   rc = cl.query( cursor ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   rc = checkLastAliveTime() ; 
   ASSERT_EQ( SDB_OK, rc ) << "wrong lastAliveTime" ;
   
   // drop cs, clean environment   
   rc = db.dropCollectionSpace( pCsName ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   rc = checkLastAliveTime() ; 
   ASSERT_EQ( SDB_OK, rc ) << "wrong lastAliveTime" ;
}
