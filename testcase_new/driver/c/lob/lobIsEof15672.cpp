/**************************************************************************
 * @Description:   test case for C driver
 *                  lob- seqDB-15672:isEof接口验证 
 *
 * @Modify:        wenjing wang Init
 *                 2018-09-28
**************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include <stdio.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

const char *lobContent = "abcdefghij" ; 
class lobTest15672 : public testBase
{
private:
   INT32 writeLob() ;
protected:
   void SetUp()
   {
      INT32 rc = SDB_OK ;
      sdbCSHandle cs = SDB_INVALID_HANDLE ;
      cl = SDB_INVALID_HANDLE ;
      csName = "lobtest_15672";
      clName = csName;
      testBase::SetUp() ;
      rc = sdbGetCollectionSpace( db, csName.c_str(), &cs ) ;
      if ( rc == -34 )
      {
         rc = sdbCreateCollectionSpaceV2( db, csName.c_str(), NULL, &cs ) ;
      }
      ASSERT_EQ( SDB_OK, rc ) << "fail to create/get cs " << csName ;
      
      rc = sdbGetCollection1( cs, clName.c_str(), &cl ) ;
      if ( rc == -23 )
      {
         rc = sdbCreateCollection( cs, clName.c_str(), &cl) ;
      }
      ASSERT_EQ( SDB_OK, rc ) << "fail to create/get cl " << clName  << "rc=" <<rc;
      
      if ( cs != SDB_INVALID_HANDLE )
      {
         sdbReleaseCS( cs ) ;
      }
      rc = writeLob( ) ;
      ASSERT_EQ( SDB_OK, rc ) << "writeLob failed, rc =  " << rc;
   }
   
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      if( shouldClear() && cl != SDB_INVALID_HANDLE )
      {
         rc = sdbDropCollectionSpace( db, csName.c_str() ) ; 
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
         sdbReleaseCollection( cl ) ;
      } 
      testBase::TearDown() ;
   }
protected:
   sdbCollectionHandle cl ;
   std::string csName ;
   std::string clName ;
   bson_oid_t oid ;
};

INT32 lobTest15672::writeLob()
{
   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   bson_oid_gen( &oid ) ;
   INT32 rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "sdbOpenLob failed,rc= " << rc << std::endl;
      goto done ;
   }
  
   rc = sdbWriteLob( lob, lobContent, strlen(lobContent)) ;
   if ( SDB_OK != rc )
   {
      std::cout << "sdbWriteLob failed,rc= " << rc  << std::endl;
      goto done ;
   }

   rc = sdbWriteLob( lob, lobContent, strlen(lobContent)) ;
   if ( SDB_OK != rc )
   {
      std::cout << "sdbWriteLob failed,rc= " << rc << std::endl ;
      goto done ;
   }
done:
   rc = sdbCloseLob( &lob ) ;
   std::cout << "sdbCloseLob failed,rc= " << rc ;
   
   return rc ;
}

void checkLobContent(char *readContent)
{
   INT32 ret = strncmp( readContent, lobContent, strlen(lobContent)) ;
   ASSERT_EQ( 0, ret ) << "read lob content does not match " ;
}

TEST_F( lobTest15672, isEof )
{
   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   char buff[32] ={0};  
   INT32 rc = SDB_OK ;
   UINT32 alreadyRead = 0 ;
  
   rc = sdbOpenLob( cl, &oid, SDB_LOB_READ, &lob) ;
   ASSERT_EQ( SDB_OK, rc ) << "sdbOpenLob failed,rc= " << rc ;
   rc = sdbReadLob( lob, strlen(lobContent), buff, &alreadyRead ) ;
   ASSERT_EQ( SDB_OK, rc ) << "sdbReadLob failed,rc= " << rc ;
   checkLobContent( buff ) ;
   
   BOOLEAN isEof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( isEof, FALSE ) << "sdbLobIsEof failed,rc= " << isEof ;
   
   rc = sdbReadLob( lob, strlen(lobContent), buff, &alreadyRead ) ;
   ASSERT_EQ( SDB_OK, rc ) << "sdbReadLob failed,rc= " << rc ;
   checkLobContent( buff ) ;
   
   isEof = sdbLobIsEof( lob ) ;
   ASSERT_EQ( isEof, TRUE ) << "sdbLobIsEof failed,rc= " << isEof ;
   
   rc = sdbCloseLob( &lob ) ;
   ASSERT_EQ( SDB_OK, rc ) << "sdbCloseLob failed,rc= " << rc ;
}
      
