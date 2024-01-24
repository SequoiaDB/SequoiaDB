/******************************************************************************
 * @Description   : seqDB-14406：异常启动DB主节点不影响全文索引功能，且重启后新主为新主节点
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2022.03.29
 * @LastEditTime  : 2022.05.19
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
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

   var clName = COMMCLNAME + "_ES_14406B";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { Group: groupName } );

   // 创建全文索引，插入数据
   var textIndexName = "textIndex_14406B";
   dbcl.createIndex( textIndexName, { "a": "text" } );
   var objs = new Array();
   for( var i = 0; i < 20000; i++ )
   {
      objs.push( { a: "test_14406B " + i, b: i } );
   }
   dbcl.insert( objs );

   // 异常停止数据主节点
   var preMaster = db.getRG( groupName ).getMaster();
   var preMasterNodeName = preMaster.getHostName() + ":" + preMaster.getServiceName();
   var remote = new Remote( preMaster.getHostName(), CMSVCNAME );
   var cmd = remote.getCmd();

   var doTimes = 1;
   for( ; doTimes <= 50; doTimes++ )
   {
      try
      {
         cmd.run( "ps -ef | grep sequoiadb | grep -v grep | grep " + preMaster.getServiceName() + " | awk '{print $2}' | xargs kill -9" );
      }
      catch( e )
      {
         //忽略异常
      }
      // 等待2min，检查数据组所有节点LSN是否一致
      checkGroupBusiness( 120, COMMCSNAME, clName );
      var curMaster = db.getRG( groupName ).getMaster();
      var curMasterNodeName = curMaster.getHostName() + ":" + curMaster.getServiceName();
      // 当新主非原主节点，则退出
      if( preMasterNodeName != curMasterNodeName ) 
      {
         break;
      }
      sleep( 1000 );
   }

   // 重新选主后没有切回原主，则抛异常
   if( doTimes > 50 )
   {
      throw new Error( "changePrimary fail,change primary" + preSlaveNodeName + curMasterNodeName );
   }

   // 执行增删改
   dbcl.insert( [{ a: 'test_14406B 20001', b: 20001 }, { a: 'test_14406B 20002', b: 20002 }, { a: 'test_14406B 20003', b: 20003 }] );
   dbcl.update( { $set: { a: "test_14406B update" } }, { a: "test_14406B 10001" } );
   dbcl.remove( { a: "test_14406B 10002" } );

   // 检查数据同步
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, dbcl.count() );
   checkConsistency( COMMCSNAME, clName );

   // 全文检索
   var findConf = { "$not": [{ "b": { "$gte": 10000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14406B" } } } } }] };
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
