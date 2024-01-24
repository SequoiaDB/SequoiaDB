/***************************************************
 * @Description : test case of getIndex 
 *                seqDB-16557:
 *                seqDB-16558:
                  seqDB-16559:
                  seqDB-16561:
                  seqDB-16562:
                  seqDB-16563:
 * @Modify      : wenjing wang
 *                2018-12-12
 ***************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <stdio.h>
#include "testBase.hpp"

class renameCSAndCL_16557 : public testBase
{
protected:
   const CHAR* csName1 ;
   const CHAR* csName2 ;
   const CHAR* clName1 ;
   const CHAR* clName2 ;
   sdbclient::sdbCollectionSpace cs ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      bson::BSONObj opt = BSON("a"<<1) ;
      sdbclient::sdbCollection cl ;
   
      csName1 = "renameCS16557" ;
      csName2 = "renameCS16558" ;
      clName1 = "renameCL16561" ;
      clName2 = "renameCL16562" ;

      rc = db.createCollectionSpace( csName1, 4096, cs ) ;
      if ( rc != -33 ){
         ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName1 ;
      }
      
      rc = db.createCollectionSpace( csName2, 4096, cs ) ;
      if ( rc != -33 ){
         ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName2 ;
      }
      

      rc = cs.createCollection( clName1,  cl ) ;
      if ( rc != -22 ) {
         ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName1 ;
      }
      
      rc = cs.createCollection( clName2,  cl ) ;
      if ( rc != -22 ) {
         ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName2 ;
      }
      
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = db.dropCollectionSpace( csName1 ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName1 ;
         
         rc = db.dropCollectionSpace( csName2 ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName2 ;
      }
      
      testBase::TearDown() ;
   }
} ;

TEST_F( renameCSAndCL_16557, renameCS )
{
   INT32 rc = SDB_OK ;
   sdbclient::sdbCollectionSpace csTmp ;
   const char *newCSName1 = "renameCS16559" ;
   const char *newCSName2 = "renameCS16560" ;
   //16557
   rc = db.renameCollectionSpace( csName1, newCSName1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << " renameCollectionSpace " << csName1 
                           << " to "<< newCSName1 <<  " failed" ;
   rc = db.getCollectionSpace( newCSName1, csTmp ) ;
   ASSERT_EQ( SDB_OK, rc ) << "getCollectionSpace" << newCSName1 << " failed" ;
   
   //16558
   rc = db.renameCollectionSpace( csName1, newCSName2 ) ;
   ASSERT_EQ( -34, rc ) << " renameCollectionSpace " << csName1 
                           << " to "<< newCSName2 <<  " failed" ;   
   rc = db.getCollectionSpace( newCSName2, csTmp ) ;
   ASSERT_EQ( -34, rc ) << "getCollectionSpace" << newCSName1 << " failed" ;
        
   //16559  
   rc = db.renameCollectionSpace( newCSName1, csName2 ) ;
   ASSERT_EQ( -33, rc ) << " renameCollectionSpace " << csName1 
                           << " to "<< newCSName2 <<  " failed" ;
   rc = db.getCollectionSpace( newCSName1, csTmp ) ;
   ASSERT_EQ( SDB_OK, rc ) << "getCollectionSpace" << newCSName1 << " failed" ;
   
   rc = db.renameCollectionSpace( newCSName1, csName1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << " renameCollectionSpace " << newCSName1 
                           << " to "<< csName1 <<  " failed" ;
   rc = db.getCollectionSpace( csName1, csTmp ) ;
   ASSERT_EQ( SDB_OK, rc ) << "getCollectionSpace" << newCSName1 << " failed" ;                   
}

TEST_F( renameCSAndCL_16557, renameCL )
{
   INT32 rc = SDB_OK ;
   const char *newCLName1 = "renameCl16563" ;
   const char *newCLName2 = "renameCl16564" ;
   sdbclient::sdbCollection clTmp ;
   //16561
   rc = cs.renameCollection( clName1, newCLName1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << " renameCollection " << clName1 
                           << " to "<< newCLName1 <<  " failed" ;
   rc = cs.getCollection( newCLName1, clTmp ) ;
   ASSERT_EQ( SDB_OK, rc ) << "getCollection" << newCLName1 << " failed" ;
   
   //16562
   rc = cs.renameCollection( clName1, newCLName2 ) ;
   ASSERT_EQ( -23, rc ) << " renameCollection " << clName1 
                           << " to "<< newCLName2 <<  " failed" ;   
   rc = cs.getCollection( newCLName2, clTmp ) ;
   ASSERT_EQ( -23, rc ) << "getCollection" << newCLName2 << " failed" ;
        
   //16563 
   rc = cs.renameCollection( newCLName1, clName2 ) ;
   ASSERT_EQ( -22, rc ) << " renameCollection " << newCLName1 
                           << " to "<< clName2 <<  " failed" ;
   rc = cs.getCollection( newCLName1, clTmp ) ;
   ASSERT_EQ( 0, rc ) << "getCollection" << clName2 << " failed" ;
   
   rc = cs.renameCollection( newCLName1, clName1 ) ;
   ASSERT_EQ( SDB_OK, rc ) << " renameCollection " << newCLName1 
                           << " to "<< clName1 <<  " failed" ;
   rc = cs.getCollection( clName1, clTmp ) ;
   ASSERT_EQ( SDB_OK, rc ) << "getCollection" << newCLName1 << " failed" ; 
}

