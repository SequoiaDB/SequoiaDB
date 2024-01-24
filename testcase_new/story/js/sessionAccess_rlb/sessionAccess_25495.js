/******************************************************************************
 * @Description   : seqDB-25495:设置PerferredConstraint为PrimaryOnly，指定备节点instanceid，该备节点升主 
 * @Author        : liuli
 * @CreateTime    : 2022.03.16
 * @LastEditTime  : 2022.06.21
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.csName = COMMCSNAME + "_25495";
testConf.clName = COMMCLNAME + "_25495";
testConf.useSrcGroup = true;

main( test );
function test ( args )
{
   var filePath = WORKDIR + "/lob25495/";
   var fileName = "filelob_25495";
   var fileSize = 1024 * 1024;
   var group = args.srcGroupName;

   var dbcl = args.testCL;
   insertBulkData( dbcl, 1000 );
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobID = dbcl.putLob( filePath + fileName );

   try
   {
      // 节点配置instanceid
      var instanceids = [11, 17, 32];
      setInstanceids( db, group, instanceids );

      // 获取备节点的nodeName
      var slaveNodeName = getGroupSlaveNodeName( db, group )[0];
      // 获取备节点instanceid
      var instanceid = getNodeNameInstanceid( db, slaveNodeName );

      // 修改会话属性，访问备节点，指定值节点instanceid为主节点
      var options = { "PreferredConstraint": "PrimaryOnly", "PreferredInstance": instanceid };
      db.setSessionAttr( options );

      // 查询lob
      assert.tryThrow( SDB_CLS_NOT_PRIMARY, function()
      {
         dbcl.getLob( lobID, filePath + "checkputlob25495", true );
      } );

      // 重新选主使slaveNodeName选为主节点
      var nodeInfo = slaveNodeName.split( ":" );
      var rg = db.getRG( group );
      rg.reelect( { "HostName": nodeInfo[0], "ServiceName": nodeInfo[1] } );
      commCheckBusinessStatus( db );

      // 查询lob
      dbcl.getLob( lobID, filePath + "checkputlob22856b", true );
      var actMD5 = File.md5( filePath + "checkputlob22856b" );
      assert.equal( fileMD5, actMD5 );

      // 查看访问计划，访问节点为新主节点
      explainAndCheckAccessNodes( dbcl, slaveNodeName );
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
