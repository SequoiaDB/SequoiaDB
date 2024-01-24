/******************************************************************************
 * @Description   : seqDB-25498:设置PerferredConstraint为PrimaryOnly，指定主节点instanceId，该主节点异常 
 * @Author        : liuli
 * @CreateTime    : 2022.03.17
 * @LastEditTime  : 2022.06.21
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.csName = COMMCSNAME + "_25498";
testConf.clName = COMMCLNAME + "_25498";
testConf.useSrcGroup = true;

main( test );
function test ( args )
{
   var group = args.srcGroupName;

   var dbcl = args.testCL;
   insertBulkData( dbcl, 1000 );

   try
   {
      // 节点配置instanceid
      var instanceids = [11, 17, 32];
      setInstanceids( db, group, instanceids );

      // 获取主节点的nodeName
      var masterNodeName = getGroupMasterNodeName( db, group )[0];
      // 获取主节点instanceid
      var instanceid = getNodeNameInstanceid( db, masterNodeName );

      // 修改会话属性，访问主节点，指定节点instanceid为主节点
      var options = { "PreferredConstraint": "PrimaryOnly", "PreferredInstance": instanceid };
      db.setSessionAttr( options );

      // 查询数据和访问计划
      dbcl.find().toArray();
      explainAndCheckAccessNodes( dbcl, masterNodeName );

      // 重新选主
      masterChangToSlave( db, group );

      // 多次查询数据和访问计划
      var explainNum = 10;
      for( var i = 0; i < explainNum; i++ )
      {
         assert.tryThrow( SDB_CLS_NOT_PRIMARY, function()
         {
            dbcl.find().toArray();
         } );
         assert.tryThrow( SDB_CLS_NOT_PRIMARY, function()
         {
            dbcl.find().explain();
         } );
      }
   }
   finally
   {
      deleteConf( db, { instanceid: 1 }, { GroupName: group }, SDB_RTN_CONF_NOT_TAKE_EFFECT );

      db.getRG( group ).stop();
      db.getRG( group ).start();
      commCheckBusinessStatus( db );
   }
}
