/**************************************************************************
 * @Description:   test case for C++ driver
 *                  lob- seqDB-15671:isEof接口验证 
 *
 * @Modify:        wenjing wang Init
 *                 2018-09-28
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

const char *lobContent = "abcdefghij" ; 
class lobTest15671 : public testBase
{
private:
   INT32 writeLob() ;
protected:
   void SetUp()
   {
      INT32 rc = SDB_OK ;
      csName = "lobtest_15671";
      clName = csName;
      sdbCollectionSpace cs ;
      testBase::SetUp() ;
      
      rc = db.getCollectionSpace( csName.c_str(), cs );
      if ( rc == -34 )
      {
         rc = db.createCollectionSpace( csName.c_str(), 65536, cs );
      }
      ASSERT_EQ( SDB_OK, rc ) << "fail to create/getCollectionSpace " << csName ;
      
      rc = cs.getCollection( clName.c_str(), cl );
      if ( rc == -23 )
      {
         rc = cs.createCollection( clName.c_str(), cl ) ;
      }
      ASSERT_EQ( SDB_OK, rc ) << "fail to create/getCollection " << clName ;
      rc = writeLob() ;
      ASSERT_EQ( SDB_OK, rc ) << "writeLob failed, rc =  " << rc;
   }
   
   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = db.dropCollectionSpace( csName.c_str() ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      } 
      testBase::TearDown() ;
   }
protected:
   
   sdbCollection cl ;
   std::string csName ;
   std::string clName ;
   bson::OID oid ;
   sdbLob lob ;
};

INT32 lobTest15671::writeLob()
{
   sdbLob lob ; 
   oid = bson::OID::gen() ;
   INT32 rc = cl.openLob( lob, oid, SDB_LOB_CREATEONLY ) ;
   if ( SDB_OK != rc )
   {
      std::cout << "openLob failed,rc= " << rc << std::endl;
      goto done ;
   }
  
   rc = lob.write( lobContent, strlen(lobContent)) ;
   if ( SDB_OK != rc )
   {
      std::cout << "lob write failed,rc= " << rc << std::endl;
      goto done ;
   }
   
   rc = lob.write( lobContent, strlen(lobContent)) ;
   if ( SDB_OK != rc )
   {
      std::cout << "lob write failed,rc= " << rc << std::endl;
      goto done ;
   }
done:
   rc = lob.close() ;
   if ( SDB_OK != rc )
   {
      std::cout << "lob close failed,rc= " << rc ;
   }
   return rc ;
}

void checkLobContent(char *readContent)
{
   INT32 ret = strncmp( readContent, lobContent, strlen(lobContent)) ;
   ASSERT_EQ( 0, ret ) << "read lob content does not match " ;
}

TEST_F( lobTest15671, isEof )
{
   sdbLob lob ; 
   char buff[32] ={0};  
   UINT32 alreadyRead = 0 ;
   INT32 rc = SDB_OK ;
  
   rc = cl.openLob( lob, oid, SDB_LOB_READ ) ;
   ASSERT_EQ( SDB_OK, rc ) << "openLob failed,rc= " << rc ;
   rc = lob.read( strlen(lobContent), buff, &alreadyRead ) ;
   ASSERT_EQ( SDB_OK, rc ) << "lob read failed,rc= " << rc ;
   checkLobContent( buff ) ;
   
   BOOLEAN isEof = lob.isEof(  ) ;
   ASSERT_EQ( isEof, FALSE ) << "check lob isEof failed " ;
   
   rc = lob.read( strlen(lobContent), buff, &alreadyRead ) ;
   ASSERT_EQ( SDB_OK, rc ) << "read lob failed,rc= " << rc ;
   checkLobContent( buff ) ;
   
   isEof = lob.isEof() ;
   ASSERT_EQ( isEof, TRUE ) << "check lob iseof failed" ;
   
   rc = lob.close( ) ;
   ASSERT_EQ( SDB_OK, rc ) << "close lob failed,rc= " << rc ;
}
      
