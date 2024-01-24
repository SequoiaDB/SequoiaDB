/**************************************************************
 * @Description : test case of sessionAttr
 * seqDB-19200:获取会话缓存中的事务配置
 * seqDB-19201:清空会话缓存中的事务配置
 * @Modify      : 
 *                2018-02-12
 **************************************************************/
#include <client.hpp>
#include <gtest/gtest.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;

class sessionAttrTestCache_1920 : public testBase
{
protected:

   void SetUp()
   {
      testBase::SetUp() ;
      db.getSessionAttr( orig ) ;
      cout << orig.toString() << endl;
   }
   
   void TearDown()
   {
      INT32 rc = db.updateConfig(BSON("transactiontimeout" << orig.getIntField("TransTimeout")  ), BSON("Global" << TRUE)) ;
      ASSERT_EQ( rc, SDB_OK ) ;
      testBase::TearDown() ;
   }

protected:
    BSONObj orig; 
} ;

TEST_F( sessionAttrTestCache_1920, getFromCache )
{
   
   int origTransTimeOut = orig.getIntField("TransTimeout") ;
   INT32 rc = db.setSessionAttr( BSON("TransTimeout" << origTransTimeOut + 60)) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   
   BSONObj ret ;
   rc = db.getSessionAttr( ret ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   ASSERT_EQ(ret.getIntField("TransTimeout"), origTransTimeOut + 60 );
  
   rc = db.updateConfig(BSON("transactiontimeout" << origTransTimeOut), BSON("Global" << TRUE)) ;
   ASSERT_EQ( rc, SDB_OK ) ;
 
   rc = db.getSessionAttr( ret ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   ASSERT_EQ(ret.getIntField("TransTimeout"), origTransTimeOut + 60 );   

   rc = db.getSessionAttr( ret, FALSE ) ;
   cout << ret.toString() << endl ;
   ASSERT_EQ( rc, SDB_OK ) ;
   ASSERT_EQ(ret.getIntField("TransTimeout"), origTransTimeOut + 60 );
   
}

TEST_F( sessionAttrTestCache_1920, clearCache )
{
   int origTransTimeOut = orig.getIntField("TransTimeout") ;
   INT32 rc = db.updateConfig(BSON("transactiontimeout" << origTransTimeOut + 60), BSON("Global" << TRUE)) ;
   ASSERT_EQ( rc, SDB_OK ) ;
  
   BSONObj ret ; 
   rc = db.getSessionAttr( ret ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   cout << ret.toString() << endl ;
   ASSERT_EQ(ret.getIntField("TransTimeout"), origTransTimeOut  );

   BSONObj emptyObj ;
   rc = db.setSessionAttr( emptyObj ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   
   rc = db.getSessionAttr( ret ) ;
   ASSERT_EQ( rc, SDB_OK ) ;
   cout << ret.toString() << endl ;
   ASSERT_EQ(ret.getIntField("TransTimeout"), origTransTimeOut + 60 );
}
