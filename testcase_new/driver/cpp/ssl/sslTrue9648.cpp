/********************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-1958
 *				     test ssl when usessl is true in configure file
 *				     sequoiadb need to be enterprise
 *               seqDB-9648:SSL功能开启，C++客户端使用SSL
 * @Modify:      Liang xuewang Init
 *               2017-09-22
 ********************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

static BOOLEAN origValue ;
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
   BOOLEAN val = FALSE ;
   while( (rc = cursor.next(obj)) == SDB_OK )
   {
      string svalue = obj.getField("usessl").String() ;
      if ( "TRUE"  == svalue  )
      {
         origValue = TRUE ;
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
   return rc == SDB_DMS_EOC ? SDB_OK:rc ;
}

// 测试开启ssl，sdb( useSSL=true )
TEST( sslTrueTest9648, sdbTrue9648 )
{
   INT32 rc = SDB_OK ;
   rc =  modifySSLConfig( TRUE ) ;
   ASSERT_EQ( SDB_OK, rc)  << "updateSSLConfig failed" << rc ;
   sdb db( TRUE ) ;
   rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect secure sdb when ssl is open" ;

   const CHAR* csName = "sslTestCs9648" ;
   const CHAR* clName = "sslTestCl9648" ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;

   rc = db.dropCollectionSpace( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
	
   db.disconnect() ;
   modifySSLConfig( origValue ); 
}

// 测试开启ssl，sdb( useSSL=false )
TEST( sslTrue, sdbFalse )          
{                          
   INT32 rc = SDB_OK ;
   rc =  modifySSLConfig( TRUE ) ;
   ASSERT_EQ( SDB_OK, rc)  << "updateSSLConfig failed" << rc ;
   {
      return ;
   }

   sdb db( FALSE ) ;
   rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect sdb when ssl is open" ;

   const CHAR* csName = "sslTestCs9648" ;
   const CHAR* clName = "sslTestCl9648" ; 
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   rc = createNormalCsCl( db, cs, cl, csName, clName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName << " cl " << clName ;

   rc = db.dropCollectionSpace( csName ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;

   db.disconnect() ;
   modifySSLConfig( origValue );
}
