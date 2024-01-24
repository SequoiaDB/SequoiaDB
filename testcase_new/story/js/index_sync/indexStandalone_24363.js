/******************************************************************************
 * @Description   : seqDB-24363:指定多个InstanceID创建本地索引 
 * @Author        : liuli
 * @CreateTime    : 2021.09.29
 * @LastEditTime  : 2022.04.12
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.clName = COMMCLNAME + "_24363";
testConf.useSrcGroup = true;

main( test );
function test ( args )
{
   var dbcl = args.testCL;
   var srcGroup = args.srcGroupName;
   var indexName1 = "index_24363_1";
   var indexName2 = "index_24363_2";

   insertBulkData( dbcl, 1000 );

   // 获取CL所在的节点
   var nodes = commGetGroupNodes( db, srcGroup );
   var nodeName1 = nodes[0].HostName + ":" + nodes[0].svcname;
   var nodeName2 = nodes[1].HostName + ":" + nodes[1].svcname;

   try
   {
      // 节点配置instanceid
      var instanceid1 = 11;
      var instanceid2 = 17;
      assert.tryThrow( SDB_RTN_CONF_NOT_TAKE_EFFECT, function()
      {
         db.updateConf( { instanceid: instanceid1 }, { NodeName: nodeName1 } );
      } );

      assert.tryThrow( SDB_RTN_CONF_NOT_TAKE_EFFECT, function()
      {
         db.updateConf( { instanceid: instanceid2 }, { NodeName: nodeName2 } );
      } );

      db.getRG( srcGroup ).getNode( nodeName1 ).stop();
      db.getRG( srcGroup ).getNode( nodeName1 ).start();
      db.getRG( srcGroup ).getNode( nodeName2 ).stop();
      db.getRG( srcGroup ).getNode( nodeName2 ).start();

      commCheckBusinessStatus( db );

      // 创建索引，指定的instanceid都能匹配节点
      dbcl.createIndex( indexName1, { a: 1 }, { Standalone: true }, { InstanceID: [instanceid1, instanceid2] } );
      checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName1, [nodeName1, nodeName2], true );

      // 创建索引，指定的instanceid部分无效
      dbcl.createIndex( indexName2, { c: 1 }, { Standalone: true }, { InstanceID: [instanceid1, 18] } );
      checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName2, nodeName1, true );
   }
   finally
   {
      assert.tryThrow( SDB_RTN_CONF_NOT_TAKE_EFFECT, function()
      {
         db.deleteConf( { instanceid: instanceid1 }, { NodeName: nodeName1 } );
      } );

      assert.tryThrow( SDB_RTN_CONF_NOT_TAKE_EFFECT, function()
      {
         db.deleteConf( { instanceid: instanceid2 }, { NodeName: nodeName2 } );
      } );
      db.getRG( srcGroup ).getNode( nodeName1 ).stop();
      db.getRG( srcGroup ).getNode( nodeName1 ).start();
      db.getRG( srcGroup ).getNode( nodeName2 ).stop();
      db.getRG( srcGroup ).getNode( nodeName2 ).start();
      commCheckBusinessStatus( db );
   }
}