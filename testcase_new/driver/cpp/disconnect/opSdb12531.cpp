/**************************************************************
 * @Description: opreate sdb object after disconnect
 *               seqDB-12531 : opreate sdb object after disconnect
 * @Modify     : Suqiang Ling
 *               2017-09-11
 ***************************************************************/
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

class opSdb12531 : public testBase 
{
protected:
   void SetUp() 
   {
      testBase::SetUp() ;
      db.disconnect() ;
   }

   void TearDown() {}
} ;

TEST_F( opSdb12531, opSdb )
{
   // test all interfaces of class sdb except connect(), disconnect(), isValid(), getLastAliveTime()
   // in the order of c++ api doc

   // auth
   INT32 rc = SDB_OK ;
   rc = db.createUsr( "sdbUser", "sdbPasswd" ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "create user shouldn't be executed" ;
   rc = db.removeUsr( "sdbUser", "sdbPasswd" ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "remove user shouldn't be executed" ;

   // snapshot & list
   sdbCursor cursor ;
   rc = db.getSnapshot( cursor, SDB_SNAP_SESSIONS ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get snapshot shouldn't be executed" ;
   rc = db.resetSnapshot() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "resetSnapshot shouldn't be executed" ;
   rc = db.getList( cursor, SDB_LIST_COLLECTIONS ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get list shouldn't be executed" ;

   // get cs/cl
   sdbCollection cl ;
   rc = db.getCollection( "cs.cl", cl ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get cl shouldn't be executed" ;
   sdbCollectionSpace cs ;
   rc = db.getCollectionSpace( "cs", cs ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get cs shouldn't be executed" ;

   // create/drop cs
   rc = db.createCollectionSpace( "cs", SDB_PAGESIZE_4K, cs ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "create cs shouldn't be executed" ;
   BSONObj option ;
   rc = db.createCollectionSpace( "cs", option, cs ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "create cs shouldn't be executed" ;
   rc = db.dropCollectionSpace( "cs" ) ; 
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "drop cs shouldn't be executed" ;

   // list cs/cl
   rc = db.listCollectionSpaces( cursor ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "list cs shouldn't be executed" ;
   rc = db.listCollections( cursor ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "list cl shouldn't be executed" ;

   // get RG
   rc = db.listReplicaGroups( cursor ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "list rg shouldn't be executed" ;
   sdbReplicaGroup rg ;
   rc = db.getReplicaGroup( "SYSCatalogGroup", rg ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get rg shouldn't be executed" ;
   INT32 rgID = 1 ;
   rc = db.getReplicaGroup( rgID, rg ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get rg shouldn't be executed" ;

   // modify RG
   rc = db.createReplicaGroup( "group10", rg ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "create rg shouldn't be executed" ;
   rc = db.removeReplicaGroup( "group10" ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "remove rg shouldn't be executed" ;
   BSONObj conf ;
   rc = db.createReplicaCataGroup( "hostName", "serviceName", "databasePath", conf ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "create cataRG shouldn't be executed" ;
   rc = db.activateReplicaGroup( "SYSCatalogGroup", rg ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "active rg shouldn't be executed" ;

   // SQL
   rc = db.execUpdate( "update student set age = 25 where stu_id = '01'" ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "execUpdate shouldn't be executed" ;
   rc = db.exec( "select * from foo.bar", cursor ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "exec sql shouldn't be executed" ;

   // transaction
   rc = db.transactionBegin() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "transactionBegin shouldn't be executed" ;
   rc = db.transactionCommit() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "transactionCommit shouldn't be executed" ;
   rc = db.transactionRollback() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "transactionRollback shouldn't be executed" ;

   // configure
   rc = db.flushConfigure( option ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "flush configure shouldn't be executed" ;

   // JS
   rc = db.crtJSProcedure( "xxx" ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "create js procedure shouldn't be executed" ;
   rc = db.rmProcedure( "xxx" ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "remove procedure shouldn't be executed" ;
   BSONObj cond ;
   rc = db.listProcedures( cursor, cond ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "list procedure shouldn't be executed" ;
   BSONObj errmsg ;
   SDB_SPD_RES_TYPE type = SDB_SPD_RES_TYPE_VOID ;
   rc = db.evalJS( "var db = new Sdb();", type, cursor, errmsg ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "evalJS shouldn't be executed" ;

   // backup 
   rc = db.backupOffline( option ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "backup offline shouldn't be executed" ;
   rc = db.listBackup( cursor, option ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "list backup shouldn't be executed" ;
   rc = db.removeBackup( option ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "remove backup shouldn't be executed" ;

   // task
   rc = db.listTasks( cursor ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "list tasks shouldn't be executed" ;
   SINT32 num = 3 ;
   SINT64 *taskIDs = new SINT64[num];
   rc = db.waitTasks( taskIDs, num ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "waitTasks shouldn't be executed" ;
   rc = db.cancelTask( taskIDs[0], false ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "cancelTask shouldn't be executed" ;

   // other
   BSONObj preferOpts = BSON( "PreferedInstance" << "M" ) ; 
   rc = db.setSessionAttr( preferOpts ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "setSessionAttr shouldn't be executed" ;
   rc = db.closeAllCursors() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "closeAllCursors shouldn't be executed" ;

   // domain
   sdbDomain domain ;
   rc = db.createDomain( "domainName", option, domain ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "create domain shouldn't be executed" ;
   rc = db.dropDomain( "domainName" ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "drop domain shouldn't be executed" ;
   rc = db.getDomain( "domainName", domain ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get domain shouldn't be executed" ;
   BSONObj selector,orderBy,hint ;
   rc = db.listDomains( cursor, cond, selector, orderBy, hint ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "list domain shouldn't be executed" ;

   // other
   sdbDataCenter dc ;
   rc = db.getDC( dc ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "get dc shouldn't be executed" ;
   rc = db.syncDB() ; 
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "syncDB shouldn't be executed" ;
   rc = db.analyze() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "analyze shouldn't be executed" ;
}

TEST_F( opSdb12531, opSdb1 )
{
   INT32 rc = SDB_OK ;

   // forceSession
   SINT64 sessionId = 64 ;
   rc = db.forceSession( sessionId ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "forceSession shouldn't be executed" ;
   
   // forceStepUp
   rc = db.forceStepUp() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "forceStepUp shouldn't be executed" ;

   // invalidateCache
   rc = db.invalidateCache() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "invalidateCache shouldn't be executed" ;
   
   // reloadConfig
   rc = db.reloadConfig() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "reloadConfig shouldn't be executed" ;
   
   // setPDLevel
   rc = db.setPDLevel( 5 ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "setPDLevel shouldn't be executed" ;

   // trace
   UINT32 traceBufSize = 256 ;
   rc = db.traceStart( traceBufSize ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "traceStart shouldn't be executed" ;
   rc = db.traceResume() ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "traceResume shouldn't be executed" ;
   sdbCursor cursor ;
   rc = db.traceStatus( cursor ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "traceStatus shouldn't be executed" ;
   rc = db.traceStop( NULL ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "traceStop shouldn't be executed" ;

   // msg
   const CHAR* message = "ABCDEEDCBA" ;
   rc = db.msg( message ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "msg shouldn't be executed" ;
   
   // unloadCS loadCS renameCollectionSpace
   const CHAR* csName = "testCs" ;
   rc = db.unloadCS( csName ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "unloadCS shouldn't be executed" ;
   rc = db.loadCS( csName ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "loadCS shouldn't be executed" ;
   const CHAR* newCsName = "testCs_1" ;
   rc = db.renameCollectionSpace( csName, newCsName ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) << "renameCollectionSpace shouldn't be executed" ;
}

TEST_F( opSdb12531, opSdb2 )
{
   INT32 rc = SDB_OK ;

   // updateConf deleteConf
   BSONObj config = BSON( "maxconn" << 0 ) ;
   BSONObj option = BSON( "svcname" << ARGS->svcName() ) ;
   rc = db.updateConfig( config, option ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) ;
   rc = db.deleteConfig( config, option ) ;
   EXPECT_EQ( SDB_NOT_CONNECTED, rc ) ;
}
