/************************************
*@Description: 正常启动DB备节点不影响全文索引功能
*@author:      liuxiaoxuan
*@createdate:  2019.08.21
*@testlinkCase: seqDB-14407
**************************************/
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;

main( test );

function test ()
{
   var groups = commGetGroups( db );
   for( var i = 0; i < groups.length; i++ )
   {
      var group = groups[i];
      if( group.length > 2 )
      {
         break;
      }
   }
   var groupName = group[0].GroupName;

   var clName = COMMCLNAME + "_ES_14407";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName, ReplSize: 0 } );

   // 创建全文索引，插入数据
   var textIndexName = "textIndex_14407";
   dbcl.createIndex( textIndexName, { "a": "text" } );
   var objs = new Array();
   for( var i = 0; i < 20000; i++ )
   {
      objs.push( { a: "test_14407 " + i, b: i } );
   }
   dbcl.insert( objs );

   // 正常停止数据备节点
   var preMaster = db.getRG( groupName ).getMaster();
   var preMasterNodeName = preMaster.getHostName() + ":" + preMaster.getServiceName();
   var preSlave = db.getRG( groupName ).getSlave();
   var preSlaveNodeName = preSlave.getHostName() + ":" + preSlave.getServiceName();
   try
   {
      preSlave.stop();
      preSlave.start();

      // 等待2min，检查数据组所有节点LSN是否一致
      checkGroupBusiness( 120, COMMCSNAME, clName );

      // 执行增删改
      dbcl.insert( [{ a: 'test_14407 20001', b: 20001 }, { a: 'test_14407 20002', b: 20002 }, { a: 'test_14407 20003', b: 20003 }] );
      dbcl.update( { $set: { a: "test_14407 update" } }, { a: "test_14407 10001" } );
      dbcl.remove( { a: "test_14407 10002" } );

      // 检查数据同步
      checkFullSyncToES( COMMCSNAME, clName, textIndexName, dbcl.count() );
      checkConsistency( COMMCSNAME, clName );

      // 走主节点执行全文检索
      var dbMaster = new Sdb( preMasterNodeName );
      var masterCL = dbMaster.getCS( COMMCSNAME ).getCL( clName );
      var findConf = { "$not": [{ "b": { "$gte": 10000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14407" } } } } }] };
      var actResult = dbOpr.findFromCL( masterCL, findConf, { 'a': '' }, { a: 1 } );
      var expResult = dbOpr.findFromCL( masterCL, { "b": { "$lt": 10000 } }, { 'a': '' }, { a: 1 } );
      checkResult( expResult, actResult );
      dbMaster.close();

      // 走原备节点执行全文检索
      var dbSlave = new Sdb( preSlaveNodeName );
      var slaveCL = dbSlave.getCS( COMMCSNAME ).getCL( clName );
      var actResult = dbOpr.findFromCL( slaveCL, findConf, { 'a': '' }, { a: 1 } );
      var expResult = dbOpr.findFromCL( slaveCL, { "b": { "$lt": 10000 } }, { 'a': '' }, { a: 1 } );
      checkResult( expResult, actResult );
      dbSlave.close();

      var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
      commDropCL( db, COMMCSNAME, clName, true, true );
      //SEQUOIADBMAINSTREAM-3983
      checkIndexNotExistInES( esIndexNames );
   }
   finally
   {
      preSlave.start();
   }

}
