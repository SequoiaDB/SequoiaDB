/**************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-2350
 * @Modify     : Liang xuewang Init
 *			 	     2017-04-05
 ***************************************************************/
#include <client.h>
#include <gtest/gtest.h>

TEST( invalidHandle, connectionHandle )
{
   INT32 rc = SDB_OK ;
   sdbConnectionHandle db = SDB_INVALID_HANDLE ;

   // create remove user
   rc = sdbCreateUsr( db, "testUser", "testPasswd" ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRemoveUsr( db, "testUser", "testPasswd" ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // snapshot list
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   rc = sdbGetSnapshot( db, SDB_SNAP_CONTEXTS, NULL, NULL, NULL, &cursor ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbResetSnapshot( db, NULL ) ;                                       
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetList( db, SDB_LIST_CONTEXTS, NULL, NULL, NULL, &cursor ) ;     
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // trace
   rc = sdbTraceStart( db, 1024*1024, NULL, NULL, NULL, 0 ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTraceResume( db ) ;                       
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTraceStop( db, NULL ) ;                   
   ASSERT_EQ( SDB_INVALIDARG, rc ) ; 
   rc = sdbTraceStatus( db, &cursor ) ;              
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // cs cl
   sdbCSHandle cs = SDB_INVALID_HANDLE ;
   rc = sdbCreateCollectionSpace( db, "foo", SDB_PAGESIZE_4K, &cs ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateCollectionSpaceV2( db, "foo", NULL, &cs ) ;           
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCollectionSpace( db, "foo", &cs ) ;                     
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   rc = sdbGetCollection( db, "foo.bar", &cl ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTruncateCollection( db, "foo.bar" ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDropCollectionSpace( db, "foo" ) ;    
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbListCollectionSpaces( db, &cursor ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbListCollections( db, &cursor ) ;      
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbLoadCollectionSpace( db, "foo", NULL ) ;   
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbUnloadCollectionSpace( db, "foo", NULL ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRenameCollectionSpace( db, "foo", "foo1", NULL ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;   

   // replica group
   sdbReplicaGroupHandle rg = SDB_INVALID_HANDLE ;
   rc = sdbCreateReplicaGroup( db, "testgroup", &rg ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRemoveReplicaGroup( db, "testgroup" ) ;      
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateReplicaCataGroup( db, "sdbserver1", "18800", "/opt/sequoiadb/database/cata/18800", NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetReplicaGroup( db, "SYSCoord", &rg ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetReplicaGroup1( db, 1, &rg ) ;         
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbListReplicaGroups( db, &cursor ) ;       
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // config
   rc = sdbFlushConfigure( db, NULL ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // procedure
   rc = sdbCrtJSProcedure( db, "function sum(x,y) { return x+y; }" ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRmProcedure( db, "sum" ) ;                                  
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbListProcedures( db, NULL, &cursor ) ;                       
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // eval
   SDB_SPD_RES_TYPE type; 
   bson msg; 
   bson_init(&msg);
   rc = sdbEvalJS( db, "db.foo.bar", &type, &cursor, &msg ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   bson_destroy(&msg) ;

   // close all cursors
   rc = sdbCloseAllCursors( db ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // exec
   rc = sdbExec( db, "select * from foo.bar", &cursor ) ;                             
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbExecUpdate( db, "insert into foo.bar(name,age) values('zhangshan',30)" ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // transaction
   rc = sdbTransactionBegin( db ) ;    
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTransactionCommit( db ) ;   
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbTransactionRollback( db ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // backup
   rc = sdbBackupOffline( db, NULL ) ;                          
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbListBackup( db, NULL, NULL, NULL, NULL, &cursor ) ;  
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRemoveBackup( db, NULL ) ;                           
   ASSERT_EQ( SDB_INVALIDARG, rc ) ; 

   // tasks
   rc = sdbListTasks( db, NULL, NULL, NULL, NULL, &cursor ) ;  
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   const SINT64 taskIds[] = { 2001, 2003, 2007 } ;
   rc = sdbWaitTasks( db, taskIds, 3 ) ;   
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCancelTask( db, 2003, false ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // set session attr and other
   rc = sdbSetSessionAttr( db, NULL ) ;        
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   BOOLEAN res ; 
   res = sdbIsValid( db ) ; 
   ASSERT_EQ( FALSE, res ) ;
   rc = _sdbMsg( db, "hello,who is that?" ) ;  
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbInvalidateCache( db, NULL ) ;		
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbForceSession( db, 10000, NULL ) ;   
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbForceStepUp( db, NULL ) ;           
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSyncDB( db, NULL ) ;                
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSetPDLevel( db, 5, NULL ) ;         
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbReloadConfig( db, NULL ) ;			
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;	

   // domain
   sdbDomainHandle domain = SDB_INVALID_HANDLE ;
   rc = sdbCreateDomain( db, "testdomain", NULL, &domain ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetDomain( db, "testdomain", &domain ) ;          
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDropDomain( db, "testdomain" ) ;				  
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbListDomains( db, NULL, NULL, NULL, &cursor ) ;    
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   sdbDisconnect( db ) ;
   sdbReleaseConnection( db ) ;
}

TEST( invalidHandle, csHandle )
{
   INT32 rc = SDB_OK ;
   sdbCSHandle cs = SDB_INVALID_HANDLE ;

   // cl
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;
   rc = sdbCreateCollection( cs, "bar", &cl ) ;          
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateCollection1( cs, "bar", NULL, &cl ) ;   
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCollection1( cs, "bar", &cl ) ;            
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDropCollection( cs, "bar" ) ;                 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   CHAR buff[100] ; rc = sdbGetCSName( cs, buff, 100 ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRenameCollection( cs, "bar", "bar1", NULL ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   sdbReleaseCS( cs ) ;
}

TEST( invalidHandle, clHandle )
{
   INT32 rc = SDB_OK ;
   sdbCollectionHandle cl = SDB_INVALID_HANDLE ;

   // other
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   rc = sdbGetDataBlocks( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetQueryMeta( cl, NULL, NULL, NULL, 0, -1, &cursor ) ;        
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   CHAR buff[100] ; rc = sdbGetCLName( cl, buff, 100 ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCLFullName( cl, buff, 100 ) ;              
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;	

   // alter cl,split cl
   bson option; 
   bson_init( &option ); 
   bson_append_int( &option,"ReplSize", 7 ) ; 
   bson_finish( &option ) ;
   rc = sdbAlterCollection( cl, &option ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ; 
   bson_destroy( &option ) ;
   bson start,end ; 
   bson_init( &start ) ; 
   bson_append_int( &start, "a", 0 ) ; 
   bson_finish( &start ) ;
   bson_init( &end ) ; 
   bson_append_int( &end, "a", 50 ) ;
   bson_finish( &end ) ; 
   rc = sdbSplitCollection( cl, "group1", "group2", &start, &end ) ;                      
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   SINT64 taskId ; 
   rc = sdbSplitCLAsync( cl, "group1", "group2", &start, &end, &taskId ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   bson_destroy( &start ) ; 
   bson_destroy( &end ) ;
   rc = sdbSplitCollectionByPercent( cl, "group1", "group2", 50 ) ;       
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSplitCLByPercentAsync( cl, "group1", "group2", 50, &taskId ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // index
   bson idxDef ; 
   bson_init( &idxDef ) ; 
   bson_append_int( &idxDef, "a", -1 ) ; 
   bson_finish( &idxDef ) ;
   rc = sdbCreateIndex( cl, &idxDef, "aIndex", false, false ) ;             
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateIndex1( cl, &idxDef, "aIndex", false, false, 1024*1024 ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetIndexes( cl, "aIndex", &cursor ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDropIndex( cl, "aIndex" ) ;           
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCreateIdIndex( cl, NULL ) ;           
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDropIdIndex( cl ) ;                   
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // count,insert,bulkinsert
   SINT64 count ; 
   rc = sdbGetCount( cl, NULL, &count ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetCount1( cl, NULL, NULL, &count ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   bson obj ; 
   bson_init( &obj ) ; 
   bson_append_string( &obj, "tst", "string" ) ; 
   bson_finish( &obj ) ;
   rc = sdbInsert( cl, &obj ) ;        
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbInsert1( cl, &obj, NULL ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   bson* b[1] = { &obj } ; 
   rc = sdbBulkInsert( cl, 0, b, 1 ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   bson_destroy( &obj ) ;

   // update,upsert,queryAndUpdate
   bson rule ; 
   bson_init( &rule ) ; 
   bson_append_start_object( &rule, "$set" ) ;
   bson_append_int( &rule, "age", 999 ) ; 
   bson_append_finish_object( &rule ) ; 
   bson_finish( &rule ) ;
   rc = sdbUpdate( cl, &rule, NULL, NULL ) ;        
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbUpsert( cl, &rule, NULL, NULL ) ;        
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbUpsert1( cl, &rule, NULL, NULL, NULL ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbQueryAndUpdate( cl, NULL, NULL, NULL, NULL, &rule, 0, -1, 0, true, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // delete,query,queryAndRemove,explain
   rc = sdbDelete( cl, NULL, NULL ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbQuery1( cl, NULL, NULL, NULL, NULL, 0, -1, 0, &cursor ) ;         
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbQuery( cl, NULL, NULL, NULL, NULL, 0, -1, &cursor ) ;             
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbQueryAndRemove( cl, NULL, NULL, NULL, NULL, 0, -1, 0, &cursor ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbExplain( cl, NULL, NULL, NULL, NULL, 0, 0, -1, NULL, &cursor ) ;  
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // aggregate
   bson* pObj[2] ;
   const CHAR* pArr[2] = {
      "{ $match: { $and: [ { no: { $gt: 1002 } },{ no: { $lt: 1015 } },{ dep: \"IT Academy\" } ] } }",
      "{ $project: { no: 1, \"info.name\": 1, major: 1 } }"
   } ;
   for( INT32 i = 0; i < 2; i++ ) 
   { 
      pObj[i] = bson_create() ; 
      jsonToBson( pObj[i], pArr[i] ) ; 
   }
   rc = sdbAggregate( cl, pObj, 2, &cursor ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   for( INT32 i = 0; i < 2; i++ ) 
   { 
      bson_dispose( pObj[i] ) ; 
   }

   // attach detach
   bson* opt = bson_create() ; 
   const CHAR* str = "{ \"LowBound\": { a: 1 }, \"UpBound\": { a: 100 } }" ;  
   jsonToBson( opt, str ) ;
   rc = sdbAttachCollection( cl, "foo.bar1", opt ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   bson_dispose( opt ) ;
   rc = sdbDetachCollection( cl, "foo.bar1" ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // lob
   sdbLobHandle lob = SDB_INVALID_HANDLE ;
   bson_oid_t oid ; 
   bson_oid_gen( &oid ) ;
   rc = sdbOpenLob( cl, &oid, SDB_LOB_CREATEONLY, &lob ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRemoveLob( cl, &oid ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbListLobs( cl, &cursor ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbListLobPieces( cl, &cursor ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   sdbReleaseCollection( cl ) ;
}

TEST( invalidHandle, cursorHandle )
{
   INT32 rc = SDB_OK ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;

   bson obj ; 
   bson_init( &obj ) ;
   rc = sdbNext( cursor, &obj ) ;    
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCurrent( cursor, &obj ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCloseCursor( cursor ) ;   
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   sdbReleaseCursor( cursor ) ;
}

TEST( invalidHandle, rgHandle )
{
   INT32 rc = SDB_OK ;
   sdbReplicaGroupHandle rg = SDB_INVALID_HANDLE ;

   // get rg name
   CHAR* rgName = NULL ;
   rc = sdbGetReplicaGroupName( rg, &rgName ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   CHAR buf[100] ;
   rc = sdbGetRGName( rg, buf, 100 ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // get node,start,stop
   rc = sdbStartReplicaGroup( rg ) ;   
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;	
   sdbNodeHandle node = SDB_INVALID_HANDLE ;
   rc = sdbGetNodeMaster( rg, &node ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetNodeSlave( rg, &node ) ;  
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetNodeByName( rg, "sdbserver1:11810", &node ) ;    
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetNodeByHost( rg, "sdbserver1", "11810", &node ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbStopReplicaGroup( rg ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // create node,remove node
   rc = sdbCreateNode( rg, "sdbserver1", "18800", "/opt/sequoiadb/database/coord/18800", NULL ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbAttachNode( rg, "sdbserver1", "18800", NULL ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbDetachNode( rg, "sdbserver1", "18800", NULL ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbRemoveNode( rg, "sdbserver1", "18800", NULL ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // reelect
   rc = sdbReelect( rg, NULL ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   sdbReleaseReplicaGroup( rg ) ;
}

TEST( invalidHandle, nodeHandle )
{
   INT32 rc = SDB_OK ;
   sdbNodeHandle node = SDB_INVALID_HANDLE ;

   // get node addr
   const CHAR *host, *svc, *nodename ;
   INT32 nodeId ;
   rc = sdbGetNodeAddr( node, &host, &svc, &nodename, &nodeId ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // start stop node
   rc = sdbStartNode( node ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbStopNode( node ) ;  
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   sdbReleaseNode( node ) ;
}

TEST( invalidHandle, domainHandle )
{
   INT32 rc = SDB_OK ;
   sdbDomainHandle domain = SDB_INVALID_HANDLE ;

   // alter domain
   bson opt ; 
   bson_init( &opt ) ; 
   bson_append_bool( &opt, "AutoSplit", true ) ;
   bson_finish( &opt ) ;
   rc = sdbAlterDomain( domain, &opt ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   bson_destroy( &opt ) ;

   // list cs cl group in domain
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   rc = sdbListCollectionSpacesInDomain( domain, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbListCollectionsInDomain( domain, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbListGroupsInDomain( domain, &cursor ) ;
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   sdbReleaseDomain( domain ) ;
}

TEST( invalidHandle, lobHandle )
{
   INT32 rc = SDB_OK ;
   sdbLobHandle lob = SDB_INVALID_HANDLE ;

   // write,seek,read,close
   const CHAR* writeBuf = "abcdefghijklmnopqrstuvwxyz" ;
   CHAR readBuf[100] ; 
   UINT32 readLen ;
   rc = sdbWriteLob( lob, writeBuf, strlen(writeBuf) ) ;      
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbSeekLob( lob, 0, SDB_LOB_SEEK_SET ) ;    
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbReadLob( lob, 100, readBuf, &readLen ) ; 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbCloseLob( &lob ) ;						 
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;

   // get size,create time
   SINT64 size ; 
   UINT64 mills ;
   rc = sdbGetLobSize( lob, &size ) ;         
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
   rc = sdbGetLobCreateTime( lob, &mills ) ;  
   ASSERT_EQ( SDB_INVALIDARG, rc ) ;
}
