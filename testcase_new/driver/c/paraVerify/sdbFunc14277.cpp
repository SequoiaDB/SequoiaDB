/**************************************************************************
 * @Description :   test sdb function para
 *                  seqDB-14277:connect参数校验
 *                  seqDB-14279:disconnect参数校验
 *                  seqDB-14280:create/removeUSer参数校验
 *                  seqDB-14283:get/resetSnapshot参数校验
 *                  seqDB-14284:traceStart/Resume/Stop/Status参数校验
 *                  seqDB-14285:getList参数校验
 *                  seqDB-14286:getCL/CS/RG参数校验
 *                  seqDB-14289:create/dropCS参数校验
 *                  seqDB-14290:create/removeRG参数校验
 *                  seqDB-14296:listCS/CL/RG参数校验
 *                  seqDB-14297:flushConfig参数校验
 *                  seqDB-14298:create/remove/listProcedure参数校验
 *                  seqDB-14299:evalJS参数校验
 *                  seqDB-14315:closeAllCursors参数校验
 *                  seqDB-14316:exec/execUpdate参数校验
 *                  seqDB-14317:transBegin/Commit/Rollback参数校验
 *                  seqDB-14318:releaseConnection参数校验
 *                  seqDB-14327:list/removeBakup参数校验
 *                  seqDB-14328:list/wait/cacelTask参数校验
 *                  seqDB-14329:set/getSessionAttr参数校验
 *                  seqDB-14330:isValid参数校验
 *                  seqDB-14331:msg参数校验
 *                  seqDB-14332:create/get/list/dropDomain参数校验
 *                  seqDB-14335:invalidateCache参数校验
 *                  seqDB-14336:forceSession参数校验
 *                  seqDB-14342:forceStepUp参数校验
 *                  seqDB-14343:truncateCL参数校验
 *                  seqDB-14344:pop参数校验
 *                  seqDB-14347:syncDB参数校验
 *                  seqDB-14348:load/unloadCS参数校验
 *                  seqDB-14349:setPDLevel参数校验
 *                  seqDB-14350:reloadConf参数校验
 *                  seqDB-14351:renameCS参数校验
 *                  seqDB-14352:setInterruptFunc参数校验
 *                  seqDB-14353:analyze参数校验
 *                  seqDB-14876:sdbUpdateConfig参数校验
 *                  seqDB-14877:sdbDeleteConfig参数校验
 * @Modify      :   Liang xuewang
 *                  2018-01-30
 **************************************************************************/
#include <gtest/gtest.h>
#include <client.h>
#include "testcommon.hpp"
#include "testBase.hpp"
#include "arguments.hpp"

class sdbParaVerify : public testBase
{
protected:
   const CHAR* csName ;
   sdbCSHandle cs ;

   void SetUp()  
   {
      testBase::SetUp() ;
      csName = "sdbParaVerifyTestCs" ;
      INT32 rc = SDB_OK ;
      rc = sdbCreateCollectionSpace( db, csName, SDB_PAGESIZE_4K, &cs ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
   }
   void TearDown()
   {
      INT32 rc = SDB_OK ;
      rc = sdbDropCollectionSpace( db, csName ) ;
      ASSERT_EQ( SDB_OK, rc ) ;
      sdbReleaseCS( cs ) ;
      testBase::TearDown() ;
   }
} ;

TEST_F( sdbParaVerify, connect )
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle conn = SDB_INVALID_HANDLE ;
   const CHAR* hostName = ARGS->hostName() ;
   const CHAR* svcName = ARGS->svcName() ;
   const CHAR* user = ARGS->user() ;
   const CHAR* passwd = ARGS->passwd() ;
   const CHAR* connAddrs[1] ;
   connAddrs[0] = ARGS->coordUrl() ;
   INT32 arrSize = 1 ;
   
   // test sdbConnect
   rc = sdbConnect( NULL, svcName, user, passwd, &conn ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbConnect( hostName, NULL, user, passwd, &conn ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbConnect( hostName, svcName, NULL, passwd, &conn ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbConnect( hostName, svcName, user, NULL, &conn ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbConnect( hostName, svcName, user, passwd, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // test sdbConnect1
   rc = sdbConnect1( NULL, arrSize, user, passwd, &conn ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbConnect1( connAddrs, -1, user, passwd, &conn ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbConnect1( connAddrs, 0, user, passwd, &conn ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbConnect1( connAddrs, arrSize, NULL, passwd, &conn ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbConnect1( connAddrs, arrSize, user, NULL, &conn ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbConnect1( connAddrs, arrSize, user, passwd, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // sdbSecureConnect same with sdbConnect, no need to test
   // sdbSecureConnect1 same with sdbConnect1, no need to test
}

TEST_F( sdbParaVerify, disconnect )
{
   sdbDisconnect( NULL ) ;
   sdbDisconnect( SDB_INVALID_HANDLE ) ;
   sdbDisconnect( cs ) ;
}

TEST_F( sdbParaVerify, user )
{
   INT32 rc = SDB_OK ;
   const CHAR* tmpUsr = "sdbadmin" ;
   const CHAR* tmpPasswd = "sdbadmin" ;
   const INT32 limitedLen = 256 ;
   CHAR unlimitedARG[ limitedLen + 2 ] = { 0 } ;
   memset( unlimitedARG, 'a', limitedLen + 1 ) ;

   // test sdbCreateUsr
   rc = sdbCreateUsr( NULL, tmpUsr, tmpPasswd ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateUsr( SDB_INVALID_HANDLE, tmpUsr, tmpPasswd ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateUsr( cs, tmpUsr, tmpPasswd ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbCreateUsr( db, NULL, tmpPasswd ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateUsr( db, tmpUsr, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateUsr( db, unlimitedARG, tmpPasswd );
   ASSERT_EQ( SDB_INVALIDARG, rc );
   rc = sdbCreateUsr( db, tmpUsr, unlimitedARG ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc );

   // test sdbRemoveUsr
   rc = sdbRemoveUsr( NULL, tmpUsr, tmpPasswd ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRemoveUsr( SDB_INVALID_HANDLE, tmpUsr, tmpPasswd ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRemoveUsr( cs, tmpUsr, tmpPasswd ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbRemoveUsr( db, tmpUsr, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRemoveUsr( db, NULL, tmpPasswd ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( sdbParaVerify, snapshot )
{
   INT32 rc = SDB_OK ;
   INT32 snapType = SDB_SNAP_CONTEXTS ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   
   // test sdbGetSnapshot
   rc = sdbGetSnapshot( NULL, snapType, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetSnapshot( SDB_INVALID_HANDLE, snapType, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetSnapshot( cs, snapType, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   // test meaningless, remove
   // rc = sdbGetSnapshot( db, 14, NULL, NULL, NULL, &cursor ) ;
   // ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetSnapshot( db, snapType, NULL, NULL, NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // test sdbResetSnapshot
   rc = sdbResetSnapshot( NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbResetSnapshot( SDB_INVALID_HANDLE, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbResetSnapshot( cs, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
}

TEST_F( sdbParaVerify, trace )
{
   INT32 rc = SDB_OK ;
   UINT32 traceBufSize = 1000 ;
   UINT32 nTids = 0 ;

   // test sdbTraceStart
   rc = sdbTraceStart( NULL, traceBufSize, NULL, NULL, NULL, nTids ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTraceStart( SDB_INVALID_HANDLE, traceBufSize, NULL, NULL, NULL, nTids ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTraceStart( cs, traceBufSize, NULL, NULL, NULL, nTids ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;

   // test sdbTraceResume
   rc = sdbTraceResume( NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTraceResume( SDB_INVALID_HANDLE ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTraceResume( cs ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;

   // test sdbTraceStop
   rc = sdbTraceStop( NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTraceStop( SDB_INVALID_HANDLE, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTraceStop( cs, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   
   // test sdbTraceStatus
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   rc = sdbTraceStatus( NULL, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTraceStatus( SDB_INVALID_HANDLE, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTraceStatus( cs, &cursor ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbTraceStatus( db, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( sdbParaVerify, list )
{
   INT32 rc = SDB_OK ;
   INT32 listType = SDB_LIST_CONTEXTS ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   
   // test sdbGetList
   rc = sdbGetList( NULL, listType, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetList( SDB_INVALID_HANDLE, listType, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetList( cs, listType, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbGetList( db, 13, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetList( db, listType, NULL, NULL, NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( sdbParaVerify, getCsClRg )
{
   INT32 rc = SDB_OK ;
   const INT32 maxLen = 127 ;

   // test sdbGetCollection
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   const CHAR* clFullName = "foo.bar" ;
   CHAR longClFullName[ maxLen*2+3 ] = { 0 } ;
   memset( longClFullName, 'x', maxLen*2+2 ) ;
   rc = sdbGetCollection( NULL, clFullName, &cl ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCollection( SDB_INVALID_HANDLE, clFullName, &cl ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCollection( cs, clFullName, &cl ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbGetCollection( db, NULL, &cl ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCollection( db, longClFullName, &cl ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCollection( db, clFullName, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // test sdbGetCollectionSpace
   sdbCSHandle testCs = SDB_INVALID_HANDLE ;
   CHAR longCsName[ maxLen+2 ] = { 0 } ;
   memset( longCsName, 'x', maxLen+1 ) ;
   rc = sdbGetCollectionSpace( NULL, csName, &testCs ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCollectionSpace( SDB_INVALID_HANDLE, csName, &testCs ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCollectionSpace( cs, csName, &testCs ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbGetCollectionSpace( db, longCsName, &testCs ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCollectionSpace( db, NULL, &testCs ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCollectionSpace( db, csName, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // test sdbGetReplicaGroup
   sdbReplicaGroupHandle rg = SDB_INVALID_HANDLE ;
   const CHAR* rgName = "tmpRg" ;
   CHAR longRgName[ maxLen+2 ] = { 0 } ;
   memset( longRgName, 'x', maxLen+1 ) ;
   rc = sdbGetReplicaGroup( NULL, rgName, &rg ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetReplicaGroup( SDB_INVALID_HANDLE, rgName, &rg ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetReplicaGroup( cs, rgName, &rg ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbGetReplicaGroup( db, longRgName, &rg ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetReplicaGroup( db, NULL, &rg ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetReplicaGroup( db, rgName, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // test sdbGetReplicaGroup1
   UINT32 id = 1 ;
   rc = sdbGetReplicaGroup1( NULL, id, &rg ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetReplicaGroup1( SDB_INVALID_HANDLE, id, &rg ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetReplicaGroup1( cs, id, &rg ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbGetReplicaGroup1( db, id, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( sdbParaVerify, createDropCs )
{
   INT32 rc = SDB_OK ;
   
   // sdbCreateCollectionSpace same with sdbCreateCollectionSpaceV2, no need to test
   // test sdbCreateCollectionSpaceV2
   sdbCSHandle testCs = SDB_INVALID_HANDLE ;
   rc = sdbCreateCollectionSpaceV2( NULL, csName, NULL, &testCs ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateCollectionSpaceV2( SDB_INVALID_HANDLE, csName, NULL, &testCs ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateCollectionSpaceV2( cs, csName, NULL, &testCs ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   const INT32 maxLen = 127 ;
   CHAR longCsName[ maxLen+2 ] = { 0 } ;
   memset( longCsName, 'x', maxLen+1 ) ;
   rc = sdbCreateCollectionSpaceV2( db, NULL, NULL, &testCs ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateCollectionSpaceV2( db, longCsName, NULL, &testCs ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateCollectionSpaceV2( db, csName, NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // test sdbDropCollectionSpace
   rc = sdbDropCollectionSpace( NULL, csName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDropCollectionSpace( SDB_INVALID_HANDLE, csName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDropCollectionSpace( cs, csName ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbDropCollectionSpace( db, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDropCollectionSpace( db, longCsName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( sdbParaVerify, createRemoveRG )
{
   INT32 rc = SDB_OK ;

   // test sdbCreateReplicaGroup
   const CHAR* rgName = "tmpRg" ;
   sdbReplicaGroupHandle rg = SDB_INVALID_HANDLE ;
   rc = sdbCreateReplicaGroup( NULL, rgName, &rg ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateReplicaGroup( SDB_INVALID_HANDLE, rgName, &rg ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateReplicaGroup( cs, rgName, &rg ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbCreateReplicaGroup( db, NULL, &rg ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   const INT32 maxLen = 127 ;
   CHAR longRgName[ maxLen+2 ] = { 0 } ;
   memset( longRgName, 'x', maxLen+1 ) ;
   rc = sdbCreateReplicaGroup( db, longRgName, &rg ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateReplicaGroup( db, rgName, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // test sdbRemoveReplicaGroup
   rc = sdbRemoveReplicaGroup( NULL, rgName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRemoveReplicaGroup( SDB_INVALID_HANDLE, rgName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRemoveReplicaGroup( cs, rgName ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbRemoveReplicaGroup( db, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRemoveReplicaGroup( db, longRgName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRemoveReplicaGroup( db, "" ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
  
   // test sdbCreateReplicaCataGroup
   const CHAR* hostName = "sdbserver1" ;
   const CHAR* svcName = "11800" ;
   const CHAR* dbPath = "/opt/sequoiadb/database/cata/11800" ;
   rc = sdbCreateReplicaCataGroup( NULL, hostName, svcName, dbPath, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateReplicaCataGroup( SDB_INVALID_HANDLE, hostName, svcName, dbPath, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateReplicaCataGroup( cs, hostName, svcName, dbPath, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbCreateReplicaCataGroup( db, NULL, svcName, dbPath, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateReplicaCataGroup( db, hostName, NULL, dbPath, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateReplicaCataGroup( db, hostName, svcName, NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( sdbParaVerify, listCsClRg )
{
   // sdbListCollectionSpaces sdbListCollections sdbListReplicaGroups 
   // same with sdbGetList, no need to test
}

TEST_F( sdbParaVerify, flushConf )
{
   INT32 rc = SDB_OK ;

   // test sdbFlushConfigure
   rc = sdbFlushConfigure( NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbFlushConfigure( SDB_INVALID_HANDLE, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbFlushConfigure( cs, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
}

TEST_F( sdbParaVerify, procedure )
{
   INT32 rc = SDB_OK ;

   // test sdbCrtJSProcedure
   const CHAR* code = "function sum(x, y) { return x+y ; }" ;
   rc = sdbCrtJSProcedure( NULL, code ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;  
   rc = sdbCrtJSProcedure( SDB_INVALID_HANDLE, code ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCrtJSProcedure( cs, code ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbCrtJSProcedure( db, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // test sdbRmProcedure
   const CHAR* spName = "sum" ;
   rc = sdbRmProcedure( NULL, spName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;  
   rc = sdbRmProcedure( SDB_INVALID_HANDLE, spName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRmProcedure( cs, spName ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbRmProcedure( db, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( sdbParaVerify, evalJS )
{
   INT32 rc = SDB_OK ;

   // test sdbEvalJS
   const CHAR* code = "db.foo" ;
   SDB_SPD_RES_TYPE type ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   rc = sdbEvalJS( NULL, code, &type, &cursor, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbEvalJS( SDB_INVALID_HANDLE, code, &type, &cursor, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbEvalJS( cs, code, &type, &cursor, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbEvalJS( db, NULL, &type, &cursor, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbEvalJS( db, code, NULL, &cursor, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbEvalJS( db, code, &type, NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( sdbParaVerify, closeAllCursors )
{
   INT32 rc = SDB_OK ;

   // test sdbCloseAllCursors
   rc = sdbCloseAllCursors( NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCloseAllCursors( SDB_INVALID_HANDLE ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCloseAllCursors( cs ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
}

TEST_F( sdbParaVerify, execAndExecUpdate )
{
   INT32 rc = SDB_OK ;
   const CHAR* updateSql = "update foo.bar set age=30 where age<25" ;
   const CHAR* selectSql = "select age,name from foo.bar" ;
   const CHAR* listSql = "list collections" ;
   
   // test sdbExec
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   rc = sdbExec( NULL, selectSql, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbExec( SDB_INVALID_HANDLE, selectSql, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbExec( cs, selectSql, &cursor ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbExec( db, updateSql, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbExec( db, NULL, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbExec( db, selectSql, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // test sdbExecUpdate
   rc = sdbExecUpdate( NULL, updateSql ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbExecUpdate( SDB_INVALID_HANDLE, updateSql ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbExecUpdate( cs, updateSql ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbExecUpdate( db, selectSql ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbExecUpdate( db, listSql ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbExecUpdate( db, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( sdbParaVerify, trans )
{
   INT32 rc = SDB_OK ;

   // test sdbTransactionBegin
   rc = sdbTransactionBegin( NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTransactionBegin( SDB_INVALID_HANDLE ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTransactionBegin( cs ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;

   // test sdbTransactionCommit
   rc = sdbTransactionCommit( NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTransactionCommit( SDB_INVALID_HANDLE ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTransactionCommit( cs ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
  
   // test sdbTransactionRollback
   rc = sdbTransactionRollback( NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTransactionRollback( SDB_INVALID_HANDLE ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTransactionRollback( cs ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
}

TEST_F( sdbParaVerify, releaseConn )
{
   // test sdbReleaseConnection
   sdbReleaseConnection( NULL ) ;
   sdbReleaseConnection( SDB_INVALID_HANDLE ) ;
   sdbReleaseConnection( cs ) ;
}

TEST_F( sdbParaVerify, backup )
{
   INT32 rc = SDB_OK ;

   // test sdbBackupOffline
   rc = sdbBackupOffline( NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbBackupOffline( SDB_INVALID_HANDLE, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbBackupOffline( cs, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   
   // test sdbListBackup
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   rc = sdbListBackup( NULL, NULL, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbListBackup( SDB_INVALID_HANDLE, NULL, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbListBackup( cs, NULL, NULL, NULL, NULL, &cursor ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbListBackup( db, NULL, NULL, NULL, NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
  
   // test sdbRemoveBackup
   rc = sdbRemoveBackup( NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRemoveBackup( SDB_INVALID_HANDLE, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRemoveBackup( cs, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
}

TEST_F( sdbParaVerify, task )
{
   INT32 rc = SDB_OK ;

   // sdbListTasks same with sdbGetList, no need to test
   // test sdbWaitTasks
   const SINT64 taskIDs[1] = { 29 } ;
   SINT32 num = 1 ;
   rc = sdbWaitTasks( NULL, taskIDs, num ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbWaitTasks( SDB_INVALID_HANDLE, taskIDs, num ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbWaitTasks( cs, taskIDs, num ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbWaitTasks( db, NULL, num ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbWaitTasks( db, taskIDs, -1 ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // test sdbCancelTask
   rc = sdbCancelTask( NULL, taskIDs[0], FALSE ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCancelTask( SDB_INVALID_HANDLE, taskIDs[0], FALSE ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCancelTask( cs, taskIDs[0], FALSE ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbCancelTask( db, 0, FALSE ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCancelTask( db, -1, FALSE ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( sdbParaVerify, sessionAttr )
{
   INT32 rc = SDB_OK ;

   // test sdbSetSessionAttr
   bson option ;
   bson_init( &option ) ;
   bson_append_string( &option, "PreferedInstance", "M" ) ;
   bson_finish( &option ) ;
   rc = sdbSetSessionAttr( NULL, &option ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSetSessionAttr( SDB_INVALID_HANDLE, &option ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSetSessionAttr( cs, &option ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbSetSessionAttr( db, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   bson emptyOpt ;
   bson_init( &emptyOpt ) ;
   bson_finish( &emptyOpt ) ;
   rc = sdbSetSessionAttr( db, &emptyOpt ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &option ) ;
   bson_destroy( &emptyOpt ) ;

   // test sdbGetSessionAttr
   bson result ;
   bson_init( &result ) ;
   rc = sdbGetSessionAttr( NULL, &result ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetSessionAttr( SDB_INVALID_HANDLE, &result ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetSessionAttr( cs, &result ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbGetSessionAttr( db, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   bson_destroy( &result ) ;
}

TEST_F( sdbParaVerify, isValid )
{
   INT32 rc = SDB_OK ;
   // test sdbIsValid
   BOOLEAN result ;
   result = sdbIsValid( NULL ) ;
   ASSERT_EQ( FALSE, result ) ;
   result = sdbIsValid( SDB_INVALID_HANDLE ) ;
   ASSERT_EQ( FALSE, result ) ;
   result = sdbIsValid( cs ) ;
   ASSERT_EQ( FALSE, result) ;
}

TEST_F( sdbParaVerify, msg )
{
   INT32 rc = SDB_OK ;
   // test _sdbMsg
   const CHAR* msg = "test _sdbMsg" ;
   rc = _sdbMsg( NULL, msg ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = _sdbMsg( SDB_INVALID_HANDLE, msg ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = _sdbMsg( cs, msg ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = _sdbMsg( db, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( sdbParaVerify, domain )
{
   INT32 rc = SDB_OK ;
   if( isStandalone( db ) )
   {
      printf( "Run mode is standalone\n" ) ;
      return ;
   }

   // test sdbCreateDomain
   const CHAR* domainName = "testDomain14332" ;
   sdbDomainHandle domain ;
   rc = sdbCreateDomain( NULL, domainName, NULL, &domain ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateDomain( SDB_INVALID_HANDLE, domainName, NULL, &domain ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateDomain( cs, domainName, NULL, &domain ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbCreateDomain( db, NULL, NULL, &domain ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   const INT32 maxLen = 127 ;
   CHAR longDomainName[ maxLen+2 ] = { 0 } ;
   memset( longDomainName, 'x', maxLen+1 ) ;
   rc = sdbCreateDomain( db, longDomainName, NULL, &domain ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateDomain( db, domainName, NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // test sdbDropDomain
   rc = sdbDropDomain( NULL, domainName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDropDomain( SDB_INVALID_HANDLE, domainName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDropDomain( cs, domainName ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbDropDomain( db, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDropDomain( db, longDomainName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // test sdbGetDomain
   rc = sdbGetDomain( NULL, domainName, &domain ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetDomain( SDB_INVALID_HANDLE, domainName, &domain ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetDomain( cs, domainName, &domain ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbGetDomain( db, NULL, &domain ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetDomain( db, longDomainName, &domain ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetDomain( db, "notExistDomain", &domain ) ;
   ASSERT_EQ( SDB_CAT_DOMAIN_NOT_EXIST, rc ) ;
   rc = sdbGetDomain( db, domainName, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // sdbListDomains same with sdbGetList, no need to test 
}

TEST_F( sdbParaVerify, invalidateCache )
{
   INT32 rc = SDB_OK ;
   // test sdbInvalidateCache
   rc = sdbInvalidateCache( NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbInvalidateCache( SDB_INVALID_HANDLE, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbInvalidateCache( cs, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
}

TEST_F( sdbParaVerify, forceSession )
{
   INT32 rc = SDB_OK ;
   // test sdbForceSession
   SINT64 sessionID = 27 ;
   rc = sdbForceSession( NULL, sessionID, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbForceSession( SDB_INVALID_HANDLE, sessionID, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbForceSession( cs, sessionID, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
}

TEST_F( sdbParaVerify, forceStepUp )
{
   INT32 rc = SDB_OK ;
   
   // test sdbForceStepUp
   rc = sdbForceStepUp( NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbForceStepUp( SDB_INVALID_HANDLE, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbForceStepUp( cs, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ; 
}

TEST_F( sdbParaVerify, truncateCl )
{
   INT32 rc = SDB_OK ;

   // test sdbTruncateCollection
   const CHAR* clFullName = "foo.bar" ;
   rc = sdbTruncateCollection( NULL, clFullName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTruncateCollection( SDB_INVALID_HANDLE, clFullName ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTruncateCollection( cs, clFullName ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbTruncateCollection( db, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTruncateCollection( db, "" ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( sdbParaVerify, pop )
{
   INT32 rc = SDB_OK ;

   // test sdbPop
   //const CHAR* clFullName = "foo.bar" ;
   rc = sdbPop( NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbPop( SDB_INVALID_HANDLE,  NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbPop( cs,  NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbPop( db, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
}

TEST_F( sdbParaVerify, sync )
{
   INT32 rc = SDB_OK ;

   // test sdbSyncDB
   rc = sdbSyncDB( NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSyncDB( SDB_INVALID_HANDLE, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSyncDB( cs, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
}

TEST_F( sdbParaVerify, loadUnloadCs )
{
   INT32 rc = SDB_OK ;

   // test sdbLoadCollectionSpace
   const CHAR* tmpCsName = "tmpCs14348" ;
   rc = sdbLoadCollectionSpace( NULL, tmpCsName, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbLoadCollectionSpace( SDB_INVALID_HANDLE, tmpCsName, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbLoadCollectionSpace( cs, tmpCsName, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   // rc = sdbLoadCollectionSpace( db, NULL, NULL ) ;  // core, wait to make questionare
   // ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // test sdbUnloadCollectionSpace
   rc = sdbUnloadCollectionSpace( NULL, tmpCsName, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbUnloadCollectionSpace( SDB_INVALID_HANDLE, tmpCsName, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbUnloadCollectionSpace( cs, tmpCsName, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbUnloadCollectionSpace( db, NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( sdbParaVerify, setPDLevel )
{
   INT32 rc = SDB_OK ;

   // test sdbSetPDLevel
   INT32 pdLevel = 5 ;
   rc = sdbSetPDLevel( NULL, pdLevel, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSetPDLevel( SDB_INVALID_HANDLE, pdLevel, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSetPDLevel( cs, pdLevel, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
}

TEST_F( sdbParaVerify, reloadConf )
{
   INT32 rc = SDB_OK ;

   // test sdbReloadConfig
   rc = sdbReloadConfig( NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbReloadConfig( SDB_INVALID_HANDLE, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbReloadConfig( cs, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
}

TEST_F( sdbParaVerify, renameCs )
{
   INT32 rc = SDB_OK ;

   // test sdbRenameCollectionSpace
   const CHAR* oldCsName = "oldCs14351" ;
   const CHAR* newCsName = "newCs14351" ;
   rc = sdbRenameCollectionSpace( NULL, oldCsName, newCsName, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRenameCollectionSpace( SDB_INVALID_HANDLE, oldCsName, newCsName, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRenameCollectionSpace( cs, oldCsName, newCsName, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
   rc = sdbRenameCollectionSpace( db, NULL, newCsName, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRenameCollectionSpace( db, oldCsName, NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}

TEST_F( sdbParaVerify, setInterrputFunc )
{
   INT32 rc = SDB_OK ;

   // test sdbSetConnectionInterruptFunc
   sdbSetConnectionInterruptFunc( NULL, NULL ) ;
   sdbSetConnectionInterruptFunc( SDB_INVALID_HANDLE, NULL ) ;
   sdbSetConnectionInterruptFunc( cs, NULL ) ;
}

TEST_F( sdbParaVerify, analyze )
{
   INT32 rc = SDB_OK ;

   // test sdbAnalyze
   rc = sdbAnalyze( NULL, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbAnalyze( SDB_INVALID_HANDLE, NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbAnalyze( cs, NULL ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;
}

TEST_F( sdbParaVerify, updateConf )
{
   INT32 rc = SDB_OK ;

   // test sdbUpdateConfig
   bson config ;
   bson_init( &config ) ;
   bson_append_int( &config, "maxconn", 0 ) ;
   bson_finish( &config ) ;
   bson option ;
   bson_init( &option ) ;
   bson_append_string( &option, "svcname", ARGS->svcName() ) ;
   bson_finish( &option ) ;
   rc = sdbUpdateConfig( NULL, &config, &option ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbUpdateConfig( SDB_INVALID_HANDLE, &config, &option ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbUpdateConfig( cs, &config, &option ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;

   rc = sdbUpdateConfig( db, NULL, &option ) ;
   ASSERT_EQ( SDB_DRIVER_BSON_ERROR, rc ) ;  // TODO: better return SDB_INVALIDARG
   rc = sdbUpdateConfig( db, &config, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &config ) ;
   bson_destroy( &option ) ;
}

TEST_F( sdbParaVerify, deleteConf )
{
   INT32 rc = SDB_OK ;

   // test sdbDeleteConfig
   bson config ;
   bson_init( &config ) ;
   bson_append_int( &config, "maxconn", 1 ) ;
   bson_finish( &config ) ;
   bson option ;
   bson_init( &option ) ;
   bson_append_string( &option, "svcname", ARGS->svcName() ) ;
   bson_finish( &option ) ;
   rc = sdbDeleteConfig( NULL, &config, &option ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDeleteConfig( SDB_INVALID_HANDLE, &config, &option ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDeleteConfig( cs, &config, &option ) ;
   ASSERT_EQ( SDB_CLT_INVALID_HANDLE, rc ) ;

   rc = sdbDeleteConfig( db, NULL, &option ) ;
   ASSERT_EQ( SDB_DRIVER_BSON_ERROR, rc ) ;  // TODO: better return SDB_INVALIDARG
   rc = sdbDeleteConfig( db, &config, NULL ) ;
   ASSERT_EQ( SDB_OK, rc ) ;
   bson_destroy( &config ) ;
   bson_destroy( &option ) ;
}
