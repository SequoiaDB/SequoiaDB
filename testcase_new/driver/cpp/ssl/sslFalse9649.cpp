/******************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-1958
 *               seqDB-9649:SSL功能未开启，C++客户端使用SSL
 *				     test ssl when usessl is false in configure file
 * @Modify:      Liang xuewang Init
 *               2017-07-20
 ******************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

static BOOLEAN origValue = FALSE ;
static sdb db ;

static INT32 modifySSLConfig(BOOLEAN value)
{
   sdb db ;
   INT32 rc ;
   if ( db.isValid() == FALSE )
   {
      rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
      if ( rc != SDB_OK )
      {
         return rc;
      }
   }
   
   sdbCursor cursor ;
   rc = db.getSnapshot(cursor, SDB_SNAP_CONFIGS, BSON("role"<<"coord"), BSON("usessl"<<"")) ;
   if ( rc != SDB_OK )
   {
      return rc ;
   }
   
   BSONObj obj;
   while( (rc = cursor.next(obj)) == SDB_OK )
   {
      string svalue = obj.getField("usessl").String() ;
      if ( "TRUE"  == svalue  )
      {
         origValue = TRUE ;
      }
      else
      {
         origValue = FALSE ;
      }

      if ( value != origValue )
      {
         break;
      }
   }
   
   if ( origValue != value )
   {
      rc = db.updateConfig(BSON("usessl" << value), BSON("role"<<"coord"));
   }
   return rc == SDB_DMS_EOC ? SDB_OK: rc;
}


// 测试关闭ssl，sdb( useSSL=false )
TEST( sslFalseTest9649, sdbFalse9649 )
{
   INT32 rc = SDB_OK ;
   rc = modifySSLConfig(FALSE) ;
   ASSERT_EQ( SDB_OK, rc)  << "updateSSLConfig failed" << rc ;
   sdb db( FALSE ) ;
   rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb when ssl closed" ;

   const CHAR* csName = "sslTestCs9649" ;
   const CHAR* clName = "sslTestCl9649" ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;

   rc = db.dropCollectionSpace( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
	
   db.disconnect() ;
   modifySSLConfig( origValue ) ;
}

// 测试关闭ssl，sdb( useSSL=true )
TEST( sslFalse, sdbTrue )
{
   INT32 rc = SDB_OK ;

   rc = modifySSLConfig(FALSE) ;
   ASSERT_EQ( SDB_OK, rc)  << "updateSSLConfig failed" << rc ;
   sdb db( TRUE ) ;
   rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
   ASSERT_EQ( SDB_NETWORK, rc ) << "fail to test connect secure sdb when ssl closed" ;
   modifySSLConfig(origValue) ;
}
