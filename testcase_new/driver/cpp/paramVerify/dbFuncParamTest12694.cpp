/*****************************************************************************
 * @Description: parameter verification for class sdb
 *               seqDB-12694 : connect() parameter verification
 *               seqDB-12695 : createUsr() parameter verification
 *               seqDB-12696 : getSnapshot() parameter verification
 *               seqDB-12698 : getList() parameter verification
 *               seqDB-12699 : getCollection() parameter verification
 *               seqDB-12700 : getCollectionSpace() parameter verification
 *               seqDB-12701 : createCollectionSpace() parameter verification
 *               seqDB-12702 : dropCollectionSpace() parameter verification
 *               seqDB-12703 : getReplicaGroup() parameter verification
 *               seqDB-12704 : createReplicaGroup() parameter verification
 *               seqDB-12705 : removeReplicaGroup() parameter verification
 *               seqDB-12706 : createReplicaCataGroup() parameter verification
 *               seqDB-12707 : activateReplicaGroup() parameter verification
 *               seqDB-12708 : execUpdate() parameter verification
 *               seqDB-12709 : exec() parameter verification
 *               seqDB-12710 : crtJSProcedure() parameter verification
 *               seqDB-12711 : rmProcedure() parameter verification
 *               seqDB-12712 : evalJS() parameter verification
 *               seqDB-12713 : waitTasks() parameter verification
 *               seqDB-12714 : cancelTask() parameter verification
 *               seqDB-12715 : setSessionAttr() parameter verification
 *               seqDB-12716 : createDomain() parameter verification
 *               seqDB-12717 : dropDomain() parameter verification
 *               seqDB-12718 : getDomain() parameter verification
 *               seqDB-14686 : traceStop参数校验( 在trace正常用例覆盖)
 *               seqDB-14687 : msg参数校验
 *               seqDB-14688 : loadCS参数校验
 *               seqDB-14689 : unloadCS参数校验
 *               seqDB-14690 : renameCollectionSpace参数校验
 * @Modify     : Suqiang Ling
 *               2017-09-11
 *******************************************************************************/
#include <gtest/gtest.h>
#include <client.hpp>
#include <iostream>
#include <vector>
#include "testcommon.hpp"
#include "arguments.hpp"
#include "testBase.hpp"

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

class dbFuncParamTest : public testBase {} ;

TEST_F( dbFuncParamTest, connect12694 )
{
   INT32 rc = SDB_OK ;
   const CHAR *pConnAddrs[1] ;
   pConnAddrs[0] = ARGS->coordUrl() ;

   // pHostName NULL 
   rc = db.connect( NULL, ARGS->port() ) ; 
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   rc = db.connect( NULL, ARGS->port() , ARGS->user(), ARGS->passwd() ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   rc = db.connect( NULL, ARGS->svcName() ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   rc = db.connect( NULL, ARGS->svcName(), ARGS->user(), ARGS->passwd() ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;

   // pServiceName NULL 
   const CHAR *pNullSvcName = NULL ;
   rc = db.connect( ARGS->hostName(), pNullSvcName ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   rc = db.connect( ARGS->hostName(), pNullSvcName, ARGS->user(), ARGS->passwd() ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;

   // pUsrName NULL
   rc = db.connect( ARGS->hostName(), ARGS->port() , NULL, ARGS->passwd() ) ;
   EXPECT_EQ( 0, rc );
   // pPasswd NULL
   rc = db.connect( ARGS->hostName(), ARGS->port() , ARGS->user(), NULL ) ;
   EXPECT_EQ( 0, rc );
   // pUsrName && pPasswd NULL
   rc = db.connect( ARGS->hostName(), ARGS->port() , NULL, NULL ) ;
   EXPECT_EQ( 0, rc );
   //EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   //rc = db.connect( ARGS->hostName(), ARGS->svcName(), NULL, ARGS->passwd() ) ;
   //EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   rc = db.connect( pConnAddrs, 1, NULL, ARGS->passwd() ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;

//   // pPasswd NULL // TODO: NULL means "", needed to be tested.
//   rc = db.connect( ARGS->hostName(), ARGS->port() , ARGS->user(), NULL ) ;
//   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
//   rc = db.connect( ARGS->hostName(), ARGS->svcName(), ARGS->user(), NULL ) ;
//   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
//   rc = db.connect( pConnAddrs, 1, ARGS->user(), NULL ) ;
//   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   // invalid pConnAddrs
   const CHAR **pNullConnAddrs = NULL ;
   rc = db.connect( pNullConnAddrs, 1, ARGS->user(), ARGS->passwd() ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;

   const CHAR *pInvalidUrlConnAddrs[1] ;
   pInvalidUrlConnAddrs[0] = "localhost11810" ;
   rc = db.connect( pInvalidUrlConnAddrs, 1, ARGS->user(), ARGS->passwd() ) ;
   EXPECT_EQ( SDB_NET_CANNOT_CONNECT, rc ) ;

   // invalid arrSize
   rc = db.connect( pConnAddrs, -1, ARGS->user(), ARGS->passwd() ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   rc = db.connect( pConnAddrs, 0, ARGS->user(), ARGS->passwd() ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, createUsr12695 )
{
   if( isStandalone( db ) )
   {
      cout << "skip this test for standalone" << endl ;
      return ;
   }

   INT32 rc = SDB_OK ;
   const INT32 limitedLen = 256 ;
   string longStr( limitedLen + 1, 'a' );

   // pUsrName NULL
   rc = db.createUsr( NULL, ARGS->passwd() ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;

   // pPasswd NULL
   rc = db.createUsr( ARGS->user(), NULL ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;

   // pPasswd empty string
   const CHAR *newUser = "user12695" ;
   rc = db.createUsr( newUser, "" ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   rc = db.removeUsr( newUser, "" ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // args exceed limited length
   rc = db.createUsr( longStr.c_str(), ARGS->passwd() ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   rc = db.createUsr( ARGS->user(), longStr.c_str() ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, removeUsr12696 )
{
   if( isStandalone( db ) )
   {
      cout << "skip this test for standalone" << endl ;
      return ;
   }

   INT32 rc = SDB_OK ;
   const CHAR *newUser = "user12696" ;
   rc = db.createUsr( newUser, "" ) ;
   ASSERT_EQ( SDB_OK, rc ) ;

   // pUsrName NULL
   rc = db.removeUsr( NULL, ARGS->passwd() ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;

   // pPasswd NULL
   rc = db.removeUsr( ARGS->user(), NULL ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;

   // pPasswd empty string
   rc = db.removeUsr( newUser, "" ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( dbFuncParamTest, getSnapshot12697 )
{
   INT32 rc = SDB_OK ;
   
   // invalid snapType
   sdbCursor cursor ;
   INT32 invalidSnapType = 9999 ;
   rc = db.getSnapshot( cursor, invalidSnapType ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( dbFuncParamTest, getList12698 )
{
   INT32 rc = SDB_OK ;
   
   // invalid snapType
   sdbCursor cursor ;
   INT32 invalidListType = 9999 ;
   rc = db.getList( cursor, invalidListType ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   rc = cursor.close() ;
   ASSERT_EQ( SDB_OK, rc ) ;
}

TEST_F( dbFuncParamTest, getCollection12699 )
{
   INT32 rc = SDB_OK ;
   sdbCollection cl ;
   rc = db.getCollection( NULL, cl ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   string longStr( 300, 'a' ) ; // must more than 255
   rc = db.getCollection( longStr.c_str(), cl ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, getCollectionSpace12700 )
{
   INT32 rc = SDB_OK ;
   sdbCollectionSpace cs ;
   rc = db.getCollectionSpace( NULL, cs ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   string longStr( 200, 'a' ) ; // must more than 127
   rc = db.getCollectionSpace( longStr.c_str(), cs ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, createCollectionSpace12701 )
{
   INT32 rc = SDB_OK ;
   sdbCollectionSpace cs ;
   BSONObj option ;
   
   // pCollectionSpaceName NULL
   rc = db.createCollectionSpace( NULL, SDB_PAGESIZE_4K, cs ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   rc = db.createCollectionSpace( NULL, option, cs ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   
   // long pCollectionSpaceName
   string longStr( 200, 'a' ) ; // must more than 127
   rc = db.createCollectionSpace( longStr.c_str(), SDB_PAGESIZE_4K, cs ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   rc = db.createCollectionSpace( longStr.c_str(), option, cs ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, dropCollectionSpace12702 )
{
   INT32 rc = SDB_OK ;
   rc = db.dropCollectionSpace( NULL ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   string longStr( 200, 'a' ) ; // must more than 127
   rc = db.dropCollectionSpace( longStr.c_str() ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, getReplicaGroup12703 )
{
   INT32 rc = SDB_OK ;
   sdbReplicaGroup result ;

   //rc = db.getReplicaGroup( NULL, result ) ;
   rc = db.getReplicaGroup( (const CHAR *)NULL, result ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   string longStr( 200, 'a' ) ; // must more than 127
   rc = db.getReplicaGroup( longStr.c_str(), result ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, createReplicaGroup12704 )
{
   INT32 rc = SDB_OK ;
   sdbReplicaGroup replicGroup ;

   rc = db.createReplicaGroup( NULL, replicGroup ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   string longStr( 200, 'a' ) ; // must more than 127
   rc = db.createReplicaGroup( longStr.c_str(), replicGroup ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, removeReplicaGroup12705 )
{
   INT32 rc = SDB_OK ;
   sdbReplicaGroup replicGroup ;

   rc = db.removeReplicaGroup( NULL ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   string longStr( 200, 'a' ) ; // must more than 127
   rc = db.removeReplicaGroup( longStr.c_str() ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, createReplicaCataGroup12706 )
{
   INT32 rc = SDB_OK ;
   BSONObj conf ;

   // pHostName NULL 
   rc = db.createReplicaCataGroup( NULL, "19000", "/tmp/", conf ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;

   // pServiceName NULL
   rc = db.createReplicaCataGroup( ARGS->hostName(), NULL, "/tmp/", conf ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   
   // pDatabasePath NULL   
   rc = db.createReplicaCataGroup( ARGS->hostName(), "11800", NULL, conf ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;

   // conf conflict
   // I don't care which was used, as long as it can run.
   conf = BSON( "svcname" << "19000" << "dbpath" << "/tmp/tmp/" ) ;
   rc = db.createReplicaCataGroup( ARGS->hostName(), ARGS->svcName(), "/tmp/", conf ) ;
   EXPECT_NE( SDB_INVALIDARG, rc ) << "fail to create cataRG when conf has svcname and dbpath" ;
   EXPECT_NE( SDB_OK, rc ) << "create cata RG shouldn't succeed" ;
}

TEST_F( dbFuncParamTest, activateReplicaGroup12707 )
{
   INT32 rc = SDB_OK ;
   // pName NULL
   sdbReplicaGroup replicGroup ;
   rc = db.activateReplicaGroup( NULL, replicGroup ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   
   // pName long string
   string longStr( 200, 'a' ) ; // must more than 127
   rc = db.activateReplicaGroup( longStr.c_str(), replicGroup ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, execUpdate12708 )
{
   INT32 rc = SDB_OK ;
   rc = db.execUpdate( NULL ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   rc = db.execUpdate( "select * from student" ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, exec12709 )
{
   INT32 rc = SDB_OK ;
   sdbCursor cursor ;

   rc = db.exec( NULL, cursor ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   rc = cursor.close() ;
   EXPECT_EQ( SDB_OK, rc ) << "fail to close cursor" ;

   rc = db.exec( "update student set age = 25 where stu_id = '01'", cursor ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   rc = cursor.close() ;
   EXPECT_EQ( SDB_OK, rc ) << "fail to close cursor" ;
}

TEST_F( dbFuncParamTest, crtJSProcedure12710 )
{
   INT32 rc = SDB_OK ;
   rc = db.crtJSProcedure( NULL ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, rmProcedure12711 )
{
   INT32 rc = SDB_OK ;
   rc = db.rmProcedure( NULL ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, evalJS12712 )
{
   INT32 rc = SDB_OK ;
   SDB_SPD_RES_TYPE type ;
   sdbCursor cursor ;
   BSONObj errmsg ;

   rc = db.evalJS( NULL, type, cursor, errmsg ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, waitTasks12713 )
{
   INT32 rc = SDB_OK ;

   // taskIDs NULL
   rc = db.waitTasks( NULL, 1 ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;

   // num zero
   SINT64 *taskIDs = new SINT64[1];
   rc = db.waitTasks( taskIDs, 0 ) ;
   EXPECT_EQ( SDB_OK, rc ) ; // empty is ok

   // num nagetive
   rc = db.waitTasks( taskIDs, -1 ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, cancelTask12714 )
{
   INT32 rc = SDB_OK ;

   rc = db.cancelTask( 0, TRUE ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
   rc = db.cancelTask( -1, FALSE ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, setSessionAttr12715 )
{
   INT32 rc = SDB_OK ;
   BSONObj options ;
   rc = db.setSessionAttr( options ) ;
   EXPECT_EQ( 0, rc ) ;

   options = BSON( "preferInst" << "M" ) ;
   rc = db.setSessionAttr( options ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;

   options = BSON( "PreferedInstance" << "?" ) ; 
   rc = db.setSessionAttr( options ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, createDomain12716 )
{
   INT32 rc = SDB_OK ;
   BSONObj options ;
   sdbDomain domain ;

   rc = db.createDomain( NULL, options, domain ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;

   string longStr( 200, 'a' ) ; // must more than 127
   rc = db.createDomain( longStr.c_str(), options, domain ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, dropDomain12717 )
{
   INT32 rc = SDB_OK ;

   rc = db.dropDomain( NULL ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;

   string longStr( 200, 'a' ) ; // must more than 127
   rc = db.dropDomain( longStr.c_str() ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, getDomain12718 )
{
   INT32 rc = SDB_OK ;
   sdbDomain domain ;

   rc = db.getDomain( NULL, domain ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;

   string longStr( 200, 'a' ) ; // must more than 127
   rc = db.getDomain( longStr.c_str(), domain ) ;
   EXPECT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, msg14687 )
{
   INT32 rc = SDB_OK ;
   rc = db.msg( NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, loadCS14688 )
{
   INT32 rc = SDB_OK ;
   rc = db.loadCS( NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, unloadCS14689 )
{
   INT32 rc = SDB_OK ;
   rc = db.unloadCS( NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( dbFuncParamTest, renameCollectionSpace14690 )
{
   INT32 rc = SDB_OK ;
   const CHAR* oldName = "renameCsTest14690" ;
   const CHAR* newName = "renameCsTest14690_1" ;
   rc = db.renameCollectionSpace( NULL, newName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = db.renameCollectionSpace( oldName, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}