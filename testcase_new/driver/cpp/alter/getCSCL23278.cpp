/**************************************************************************
 * @Description:   seqDB-23278:getCS()，getCL()指定checkExist参数
 * @Modify:        liuli Init
 *                 2021-02-07
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class getCSCLTest23278 : public testBase
{
protected:
   const CHAR* csName1 ;
   const CHAR* csName2 ;
   const CHAR* clName1 ;
   const CHAR* clName2 ;
   const CHAR* fullName1 ;
   const CHAR* fullName2 ;
   const CHAR* fullName3 ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName1 = "cs_23278_1" ;
      csName2 = "cs_23278_2" ;
      clName1 = "cl_23278_1" ;
      clName2 = "cl_23278_2" ;
      fullName1 = "cs_23278_1.cl_23278_1" ;
      fullName2 = "cs_23278_1.cl_23278_2" ;
      fullName3 = "cs_23278_2.cl_23278_1" ;
      rc = db.createCollectionSpace( csName1, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName1 ;
      rc = cs.createCollection( clName1, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName1 ;
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = db.dropCollectionSpace( csName1 ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName1 ;
      }
      testBase::TearDown() ;
   }
} ;

TEST_F( getCSCLTest23278, getCS23278 )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   INT32 rc = SDB_OK ;

   // specify false, cs not exist
   sdbCollectionSpace cs1 ;
   rc = db.getCollectionSpace( csName2,cs1,false );
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs" ;

   // use cs1 create cl
   sdbCollection tcl1 ;
   rc = cs1.createCollection( clName1,tcl1 );
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc ) << "should error but success" ;

   // specify false, cs check exist
   sdbCollectionSpace cs2 ;
   rc = db.getCollectionSpace( csName1,cs2,false );
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs" ;

   // use cs2 create cl
   sdbCollection tcl2 ;
   rc = cs2.createCollection( clName2,tcl2 );
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cl" ;

   // specify true, cs check exist
   sdbCollectionSpace cs3 ;
   rc = db.getCollectionSpace( csName1,cs3,true );
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs" ;

   // use cs2 drop cl
   rc = cs2.dropCollection(clName2);
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cl" ;
   
   // specify true cs not exis
   sdbCollectionSpace cs4 ;
   rc = db.getCollectionSpace( csName2,cs4,true );
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc ) << "fail to get cs" ;
}

TEST_F( getCSCLTest23278, sdbCSGetCL23278 )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   INT32 rc = SDB_OK ;
   
   sdbCollectionSpace cs3 ;
   rc = db.getCollectionSpace( csName1,cs3,true );
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs" ;

   // SdbCS.getCollextion
   // specify false, cl not exist
   sdbCollection cl1 ;
   rc = cs3.getCollection( clName2,cl1,false );
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl" ;

   // use cl1 insert data
   BSONObj doc = BSON( "a" << 1 ) ;
   rc = cl1.insert( doc ) ;
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) << "should error but success" ;

   // specify false, cl exist
   sdbCollection cl2 ;
   rc = cs3.getCollection( clName1,cl2,false );
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl" ;

   // use cl2 insert data
   rc = cl2.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert data" ;

   // specify true, cl exist
   sdbCollection cl3 ;
   rc = cs3.getCollection( clName1,cl3,true );
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl" ;

   // use cl3 insert data
   rc = cl3.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert data" ;

   // specify true, cl not exist
   sdbCollection cl4 ;
   rc = cs3.getCollection( clName2,cl4,true );
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) << "fail to get cl" ;
}

TEST_F( getCSCLTest23278, sdbGetCL23278 )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   INT32 rc = SDB_OK ;
   
   // Sdb.getCollextion
   // specify false, cs exist, cl exist
   sdbCollection cl5 ;
   rc = db.getCollection( fullName1,cl5,false );
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl" ;

   // use cl5 insert data
   BSONObj doc = BSON( "a" << 1 ) ;
   rc = cl5.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert data" ;

   // specify false, cs exist, cl not exist
   sdbCollection cl6 ;
   rc = db.getCollection( fullName2,cl6,false );
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl" ;

   // use cl6 insert data
   rc = cl6.insert( doc ) ;
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) << "should error but success" ;

   // specify false, cs not exist
   sdbCollection cl7 ;
   rc = db.getCollection( fullName3,cl7,false );
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl" ;

   // use cl7 insert data
   rc = cl7.insert( doc ) ;
   ASSERT_EQ( SDB_DMS_CS_NOTEXIST, rc ) << "should error but success" ;

   // specify true, cs exist, cl exist
   sdbCollection cl8 ;
   rc = db.getCollection( fullName1,cl8,true );
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl" ;

   // use cl8 insert data
   rc = cl8.insert( doc ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to insert data" ;

   // specify true, cs exist, cl not exist
   sdbCollection cl9 ;
   rc = db.getCollection( fullName2,cl9,true );
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) << "fail to get cl" ;

   // specify true, cs not exist
   sdbCollection cl10 ;
   rc = db.getCollection( fullName3,cl10,true );
   ASSERT_EQ( SDB_DMS_NOTEXIST, rc ) << "fail to get cl" ;
}
