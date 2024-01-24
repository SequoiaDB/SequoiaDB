/******************************************************************************
 * @Description   : seqDB-25511:设置PerferredConstraint为SecondaryOnly，指定主节点instanceid，该主节点降备 
 * @Author        : liuli
 * @CreateTime    : 2022.03.16
 * @LastEditTime  : 2022.06.21
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.csName = COMMCSNAME + "_25511";
testConf.clName = COMMCLNAME + "_25511";
testConf.useSrcGroup = true;

main( test );
function test ( args )
{
   var filePath = WORKDIR + "/lob25511/";
   var fileName = "filelob_25511";
   var fileSize = 1024 * 1024;
   var group = args.srcGroupName;

   var dbcl = args.testCL;
   insertBulkData( dbcl, 1000 );
   deleteTmpFile( filePath );
   makeTmpFile( filePath, fileName, fileSize );
   var lobID = dbcl.putLob( filePath + fileName );

   try
   {
      // 节点配置instanceid
      var instanceids = [11, 17, 32];
      setInstanceids( db, group, instanceids );

      // 获取主节点的nodeName
      var masterNodeName = getGroupMasterNodeName( db, group )[0];
      // 获取主节点instanceid
      var instanceid = getNodeNameInstanceid( db, masterNodeName );

      // 修改会话属性，访问备节点，指定值节点instanceid为主节点
      var options = { "PreferredConstraint": "SecondaryOnly", "PreferredInstance": instanceid };
      db.setSessionAttr( options );

      // 查询lob
      assert.tryThrow( SDB_CLS_NOT_SECONDARY, function()
      {
         dbcl.getLob( lobID, filePath + "checkputlob25511", true );
      } );

      // 重新选主
      masterChangToSlave( db, group );

      // 查看访问计划，访问节点为原主节点
      explainAndCheckAccessNodes( dbcl, masterNodeName );
   }
   finally
   {
      deleteConf( db, { instanceid: 1 }, { GroupName: group }, SDB_RTN_CONF_NOT_TAKE_EFFECT );

      db.getRG( group ).stop();
      db.getRG( group ).start();
      commCheckBusinessStatus( db );
      deleteTmpFile( filePath );
   }
}