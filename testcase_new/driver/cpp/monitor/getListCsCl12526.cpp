/**************************************************************
 * @Description: get all kind of list cs, cl
 *               seqDB-12526 : get all kind of list
 * @Modify     : Suqiang Ling
 *               2017-09-11
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

class getListCsCl12526 : public testBase 
{
protected:
   const CHAR *pCsName ;
   const CHAR *pClName ;

   sdbCursor cursor ;
   BSONObj res ;

   void SetUp() 
   {
      testBase::SetUp() ;

      pCsName = "getListCsCl12526_cs" ;
      pClName = "getListCsCl12526_cl" ;
      sdbCollectionSpace cs ;
      sdbCollection cl ;

      INT32 rc = SDB_OK ;
      rc = db.createCollectionSpace( pCsName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
      rc = cs.createCollection( pClName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;
   }

   void TearDown() 
   {
      INT32 rc = SDB_OK ;
      if( shouldClear() )
      {
         rc = db.dropCollectionSpace( pCsName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
      }

      rc = cursor.close() ;
      ASSERT_EQ( SDB_OK, rc ) ;

      testBase::TearDown() ;
   }
} ;

TEST_F( getListCsCl12526, listCollections )
{
   // get list
   INT32 rc = SDB_OK ;
   string clFullNameStr = string( pCsName ) + "." + string( pClName ) ;
   BSONObj cond = BSON( "Name" << clFullNameStr.c_str() ) ;
   rc = db.getList( cursor, SDB_LIST_COLLECTIONS, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   
   // check result
   rc = cursor.next( res ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( getListCsCl12526, listCollectionSpaces )
{
   // get list
   INT32 rc = SDB_OK ;
   BSONObj cond = BSON( "Name" << pCsName ) ;
   rc = db.getList( cursor, SDB_LIST_COLLECTIONSPACES, cond ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // check result
   rc = cursor.next( res ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}
