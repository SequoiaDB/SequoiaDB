/**************************************************************************
 * @Description:   seqDB-22815:db.setSessionAttr新增CheckClientCataVersion属性
 *                 seqDB-22816:db.setSessionAttr配置CheckClientCataVersion:true，做cl. alterCollection/cl.setVersion/cl.getVersion操作
 *                 seqDB-22817:cl.setVersion参数校验
 * @Modify:        liuli Init
 *                 2020-11-21
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

class checkClientCataVersionTest22815 : public testBase
{
protected:
   const CHAR* csName ;
   const CHAR* clName ;
   const CHAR* clFullName ;
   sdbCollectionSpace cs ;
   sdbCollection cl ;
   
   void SetUp()
   {
      testBase::SetUp() ;
      INT32 rc = SDB_OK ;
      csName = "cs_22815" ;
      clName = "cl_22815" ;
      clFullName = "cs_22815.cl_22815" ;
      rc = db.createCollectionSpace( csName, SDB_PAGESIZE_4K, cs ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cs " << csName ;
      rc = cs.createCollection( clName, cl ) ;
      ASSERT_EQ( SDB_OK, rc ) << "fail to create cl " << clName ;   
   }

   void TearDown()
   {
      if( shouldClear() )
      {
         INT32 rc = db.dropCollectionSpace( csName ) ;
         ASSERT_EQ( SDB_OK, rc ) << "fail to drop cs " << csName ;
      }
      testBase::TearDown() ;
   }
} ;

TEST_F( checkClientCataVersionTest22815, checkClientCataVersion22815 )
{
   if( isStandalone( db ) )
   {
      cout << "Run mode is standalone" << endl ;
      return ;
   }

   const CHAR* catCsName ;
   const CHAR* catClName ;
   catCsName = "SYSCAT" ;
   catClName = "SYSCOLLECTIONS" ;
   INT32 rc = SDB_OK ;
   INT32 clVersion = SDB_OK;

   rc = db.setSessionAttr( BSON("CheckClientCataVersion" << false)) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get session attr" ;

   rc = db.setSessionAttr( BSON("CheckClientCataVersion" << true)) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get session attr" ;
   
   clVersion = 2147483648;
   cl.setVersion( clVersion ) ;
   rc = cl.getVersion() ;
   ASSERT_EQ( clVersion, rc ) << "fail to get version" ;
   
   BSONObj alterOpt = BSON( "AlterType " << "collection" << "Version " << 1 << "Name " << clFullName
                                         << "Alter" << BSON( "Name" << "increase version" ) ) ;
   rc = cl.alterCollection( alterOpt ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to alter cl" ;

   sdbReplicaGroup rg ;
   rc = db.getReplicaGroup( "SYSCatalogGroup", rg ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get replica group" ;

   sdbNode master ;
   rc = rg.getMaster( master ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get master" ;

   sdb node ;
   rc = master.connect( node ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to connect" ;

   sdbCollectionSpace catCs ;
   rc = node.getCollectionSpace( catCsName, catCs ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cs" ;

   sdbCollection catCl ;
   rc = catCs.getCollection( catClName, catCl );
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl" ;

   sdbCursor cursor ;
   rc = catCl.query( cursor, BSON("Name" << clFullName ) ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to query" ;

   rc = cs.getCollection( clName, cl ) ;
   ASSERT_EQ( SDB_OK, rc ) << "fail to get cl " << clName ;

   BSONObj result ;
   cursor.next( result ) ;
   rc = cl.getVersion() ;
   ASSERT_EQ( result.getField( "Version" ).Int(), rc ) << "fail to check query result" ;
   cursor.close() ;
}
