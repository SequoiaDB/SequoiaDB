/************************************
*@Description: 异常启动DB主节点不影响全文索引功能，且重启后新主为原主节点
*@author:      liuxiaoxuan
*@createdate:  2019.07.03
*@testlinkCase: seqDB-14406
**************************************/
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;
//SEQUOIADBMAINSTREAM-6439
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

   var clName = COMMCLNAME + "_ES_14406A";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName } );

   // 创建全文索引，插入数据
   var textIndexName = "textIndex_14406A";
   dbcl.createIndex( textIndexName, { "a": "text" } );
   var objs = new Array();
   for( var i = 0; i < 20000; i++ )
   {
      objs.push( { a: "test_14406A " + i, b: i } );
   }
   dbcl.insert( objs );

   // 异常停止数据主节点
   var preMaster = db.getRG( groupName ).getMaster();
   var preMasterNodeName = preMaster.getHostName() + ":" + preMaster.getServiceName();
   try
   {
      // 加大原主节点的权重，使的后面重新选举尽可能选回自己
      db.updateConf( { "weight": 100 }, { "NodeName": preMasterNodeName } );
      var remote = new Remote( preMaster.getHostName(), CMSVCNAME );
      var cmd = remote.getCmd();
      cmd.run( "ps -ef | grep sequoiadb | grep -v grep | grep " + preMaster.getServiceName() + " | awk '{print $2}' | xargs kill -9" );

      // 等待2min，检查数据组所有节点LSN是否一致
      checkGroupBusiness( 120, COMMCSNAME, clName );

      // 重新发起选主
      var doTimes = 1;
      for( ; doTimes <= 50; doTimes++ )
      {
         try
         {
            // 如果选举超时则需重新选举，这里在选举之前要先判断主节点是否存在
            isMasterNodeExist( groupName );
            db.getRG( groupName ).reelect( { Seconds: 120 } );
            // 等待选主
            isMasterNodeExist( groupName );
            var curMaster = db.getRG( groupName ).getMaster();
            var curMasterNodeName = curMaster.getHostName() + ":" + curMaster.getServiceName();
            // 当新主和原主为同一个节点，则退出
            if( preMasterNodeName == curMasterNodeName ) 
            {
               break;
            }
            sleep( 1000 );
         }
         catch( e )
         {
            if( SDB_TIMEOUT != e.message )
            {
               throw e;
            }
         }
      }

      // 选举后没有切回原主，则抛异常
      if( doTimes > 50 )
      {
         throw new Error( "changePrimary fail,reelect and change primary" + preMasterNodeName + curMasterNodeName );
      }

      // 执行增删改
      dbcl.insert( [{ a: 'test_14406A 20001', b: 20001 }, { a: 'test_14406A 20002', b: 20002 }, { a: 'test_14406A 20003', b: 20003 }] );
      dbcl.update( { $set: { a: "test_14406A update" } }, { a: "test_14406A 10001" } );
      dbcl.remove( { a: "test_14406A 10002" } );

      // 检查数据同步
      checkFullSyncToES( COMMCSNAME, clName, textIndexName, dbcl.count() );
      checkConsistency( COMMCSNAME, clName );

      // 全文检索
      var findConf = { "$not": [{ "b": { "$gte": 10000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14406A" } } } } }] };
      var actResult = dbOpr.findFromCL( dbcl, findConf, { 'a': '' } );
      var expResult = dbOpr.findFromCL( dbcl, { "b": { "$lt": 10000 } }, { 'a': '' } );
      actResult.sort( compare( "a" ) );
      expResult.sort( compare( "a" ) );
      checkResult( expResult, actResult );

      var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
      commDropCL( db, COMMCSNAME, clName, true, true );
      //SEQUOIADBMAINSTREAM-3983
      checkIndexNotExistInES( esIndexNames );
   }
   finally
   {
      // 等待环境恢复后再重置配置
      checkGroupBusiness( 120, COMMCSNAME, clName );
      db.updateConf( { "weight": 10 }, { "NodeName": preMasterNodeName } );
   }
}
