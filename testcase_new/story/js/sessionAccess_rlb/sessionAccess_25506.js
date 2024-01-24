/******************************************************************************
 * @Description   : seqDB-25506:设置PerferredConstraint为SecondaryOnly，指定多个instanceID和M 
 * @Author        : liuli
 * @CreateTime    : 2022.03.15
 * @LastEditTime  : 2022.06.21
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.csName = COMMCSNAME + "_25506";
testConf.clName = COMMCLNAME + "_25506";
testConf.useSrcGroup = true;

main( test );
function test ( args )
{
   var dbcl = args.testCL;
   insertBulkData( dbcl, 1000 );

   var group = args.srcGroupName;

   try
   {
      // 节点配置instanceid
      var instanceids = [11, 17, 32];
      setInstanceids( db, group, instanceids );
      var preferredInstance = instanceids;
      preferredInstance.push( "M" );

      var options = { "PreferredConstraint": "secondaryonly", "PreferedInstance": preferredInstance };
      // 修改会话属性，指定访问备节点，同时指定instanceid和M
      db.setSessionAttr( options );
      assert.tryThrow( SDB_CLS_NOT_SECONDARY, function()
      {
         dbcl.find().toArray();
      } );

      var options = { "PreferredConstraint": "secondaryonly", "PreferredInstance": preferredInstance };
      // 修改会话属性，指定访问备节点，同时指定instanceid和M
      db.setSessionAttr( options );
      assert.tryThrow( SDB_CLS_NOT_SECONDARY, function()
      {
         dbcl.find().toArray();
      } );
   }
   finally
   {
      deleteConf( db, { instanceid: 1 }, { GroupName: group }, SDB_RTN_CONF_NOT_TAKE_EFFECT );

      db.getRG( group ).stop();
      db.getRG( group ).start();
      commCheckBusinessStatus( db );
   }
}