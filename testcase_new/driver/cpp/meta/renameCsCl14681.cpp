/**************************************************************
 * @Description: test case for C++ driver
 *				     seqDB-14681:renameCS/CL重命名集合空间、集合
 *               only support in standalone mode
 * @Modify     : Liang xuewang Init
 *			 	     2018-03-13
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class renameCsClTest : public testBase
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

TEST_F( renameCsClTest, renameCsCl )
{
   INT32 rc = SDB_OK ;
   
   if( !isStandalone( db ) )
   {
      cout << "Run mode is not standalone" << endl ;
      return ;
   }

   const CHAR* csName = "renameCsClTestCs14681" ;
   const CHAR* clName = "renameCsClTestCl14681" ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;

   rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
   rc = cs.createCollection( clName, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;
   
   // rename cs
   const CHAR* newCsName = "renameCsClTestCs14681_1" ;
   rc = db.renameCollectionSpace( csName, newCsName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to rename cs" ;

   // get cs again
   rc = db.getCollectionSpace( newCsName, cs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs" ; 

   // rename cl
   const CHAR* newClName = "renameCsClTestCl14681_1" ;
   rc = cs.renameCollection( clName, newClName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to rename cl" ;

   rc = db.dropCollectionSpace( newCsName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
}
