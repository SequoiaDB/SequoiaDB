/******************************************************************************
 * @Description   : seqDB-24691:设置从备节点查询功能验证
 * @Author        : zhangyanan
 * @CreateTime    : 2021.11.26
 * @LastEditTime  : 2022.02.10
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;

main( test )
function test ()
{
   var csName = "sessionAccess_24691";
   var clName = "sessionAccess_24691";
   var options = { SkipRecycleBin: true };
   commDropCS( db, csName, true, "", options );

   var dbcl = commCreateCL( db, csName, clName );
   var groupName = commGetCLGroups( db, csName + "." + clName );
   var srcGroup = groupName[0];

   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   dbcl.insert( docs );

   // 获取CL所在的节点
   var nodes = db.getRG( srcGroup ).getSlave();
   var nodeName = nodes.getHostName() + ":" + nodes.getServiceName();

   try
   {
      // 节点配置instanceid
      var instanceid = 20;
      updateConf( db, { instanceid: instanceid }, { NodeName: nodeName }, SDB_RTN_CONF_NOT_TAKE_EFFECT );

      db.getRG( srcGroup ).getNode( nodeName ).stop();
      db.getRG( srcGroup ).getNode( nodeName ).start();

      commCheckBusinessStatus( db );
      db.invalidateCache();

      // 设置PreferedStrict为instanceid
      db.setSessionAttr( { "PreferedInstance": ["S", instanceid], "PreferedStrict": true } );

      // 查询访问计划并校验节点
      var dbcl = db.getCS( csName ).getCL( clName );
      var explain = dbcl.find().explain();
      var actNodeName = explain.current().toObj()["NodeName"];
      assert.equal( actNodeName, nodeName );

      var dropClDB = new Sdb( nodes.getHostName(), nodes.getServiceName() );
      var cs = dropClDB.getCS( csName );
      cs.dropCL( clName );
      dropClDB.close();

      assert.tryThrow( -338, function()
      {
         dbcl.find().toArray();
      } );

   }
   finally
   {
      commDropCS( db, csName, true, "", options );
      deleteConf( db, { instanceid: 1 }, { NodeName: nodeName }, SDB_RTN_CONF_NOT_TAKE_EFFECT );
      db.getRG( srcGroup ).getNode( nodeName ).stop();
      db.getRG( srcGroup ).getNode( nodeName ).start();
      commCheckBusinessStatus( db );
   }
}