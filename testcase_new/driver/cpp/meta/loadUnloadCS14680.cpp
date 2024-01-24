/**************************************************************
 * @Description: test case for C++ driver
 *				     seqDB-14680:unload/loadCS卸载挂载集合空间
 * @Modify     : Liang xuewang Init
 *			 	     2018-03-12
 ***************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class loadUnloadCS14680Test : public testBase
{
protected:
   const CHAR* csName ;   

   void SetUp()
   {
      testBase::SetUp() ;
      csName = "loadUnloadCS14680TestCs" ;
      INT32 rc = SDB_OK ;
      sdbCollectionSpace cs ;
      rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs" ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = db.dropCollectionSpace( csName ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs" ;
      testBase::TearDown() ;
   }
} ;

TEST_F( loadUnloadCS14680Test, loadUnloadCS )
{
   INT32 rc = SDB_OK ;

   rc = db.unloadCS( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to unload cs" ;
   rc = db.loadCS( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to load cs" ;
}
