/******************************************************************************
 * @Description   : seqDB-24408:创建本地索引createIndex接口参数校验
 * @Author        : liuli
 * @CreateTime    : 2021.09.27
 * @LastEditTime  : 2022.04.12
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_24408";
testConf.useSrcGroup = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ( args )
{
   var dbcl = args.testCL;
   var srcGroup = args.srcGroupName;

   // 获取CL所在的节点
   var nodes = commGetGroupNodes( db, srcGroup );

   var nodeName1 = nodes[0].HostName + ":" + nodes[0].svcname;
   var nodeName2 = nodes[1].HostName + ":" + nodes[1].svcname;
   var nodeID1 = nodes[0].NodeID;
   var nodeID2 = nodes[1].NodeID;
   insertBulkData( dbcl, 5000 );
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

      // 索引名长度为1，指定节点NodeName覆盖string/array
      var indexName = "a";
      dbcl.createIndex( indexName, { "a": 1 }, { Standalone: true }, { SortBufferSize: 32, NodeName: nodeName1 } );
      checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, nodeName1, true );
      dbcl.dropIndex( indexName );
      dbcl.createIndex( indexName, { "a": 1 }, { Standalone: true }, { NodeName: [nodeName1, nodeName2] } );
      checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, [nodeName1, nodeName2], true );
      dbcl.dropIndex( indexName );

      // 索引名长度为1023，指定节点覆盖NodeId覆盖string/array
      var indexName = "";
      for( var i = 0; i < 1023; i++ )
      {
         indexName += "s";
      }
      dbcl.createIndex( indexName, { "a": 1 }, { Standalone: true }, { NodeID: nodeID1 } );
      checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, nodeName1, true );
      dbcl.dropIndex( indexName );
      dbcl.createIndex( indexName, { "a": 1 }, { Standalone: true }, { SortBufferSize: 32, NodeID: [nodeID1, nodeID2] } );
      checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, [nodeName1, nodeName2], true );
      dbcl.dropIndex( indexName );

      // 指定节点InstanceID覆盖string/array
      var indexName = "index_24408";
      dbcl.createIndex( indexName, { "a": 1 }, { Standalone: true }, { InstanceID: instanceid1 } );
      checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, nodeName1, true );
      dbcl.dropIndex( indexName );
      dbcl.createIndex( indexName, { "a": 1 }, { Standalone: true }, { InstanceID: [instanceid1, instanceid2] } );
      checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, [nodeName1, nodeName2], true );
      dbcl.dropIndex( indexName );

      // 索引名为空字符串
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         dbcl.createIndex( "", { a: 1 } );
      } );

      // 索引名以$开头
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         dbcl.createIndex( "$index", { a: 1 } );
      } );

      // 索引名长度为1024
      var indexName = "";
      for( var i = 0; i < 1024; i++ )
      {
         indexName += "t";
      }
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         dbcl.createIndex( indexName, { a: 1 } );
      } );

      var indexName = "index_24408";
      // 指定Standalone为false同时指定NodeName
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         dbcl.createIndex( indexName, { "a": 1 }, { Standalone: false }, { NodeName: nodeName1 } );
      } );

      // 指定节点NodeName为int
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         dbcl.createIndex( indexName, { "a": 1 }, { Standalone: true }, { NodeName: 17 } );
      } );
      // 指定节点NodeID和InstanceID为string
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         dbcl.createIndex( indexName, { "a": 1 }, { Standalone: true }, { NodeName: "715" } );
      } );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         dbcl.createIndex( indexName, { "a": 1 }, { Standalone: true }, { NodeName: "17" } );
      } );
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