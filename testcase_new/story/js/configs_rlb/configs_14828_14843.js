/******************************************************************************
 * @Description : test delete multi config in a group 
 *                seqDB-14828:指定一个组批量更新相同级别的配置
 *                seqDB-14843:指定一个组批量删除相同级别的配置
 * @author      : Liang XueWang 
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var nodeNum = 3;
   var groupName = "rg_14828_14843";
   var hostName = commGetGroups( db )[0][1].HostName;
   var nodeOption = { diaglevel: 3 };
   var nodes = commCreateRG( db, groupName, nodeNum, hostName, nodeOption );

   //指定diaglevel创建节点会将此参数写入节点conf文件，由于后面会校验节点conf文件里的配置参数，将此参数从conf文件里删除，以便后面校验
   var config = { "diaglevel": 1 };
   var options = { "HostName": nodes[0].hostname, "ServiceName": nodes[0].svcname.toString() };
   deleteConf( db, config, options );

   // 更新多个run级别参数
   options = { GroupName: groupName };
   configs = getConfigs( "validVal" )["runConfigs"];
   updateConf( db, configs, options );

   for( var i = 0; i < nodes.length; i++ )
   {
      var hostName = nodes[i].hostname;
      var svcName = nodes[i].svcname;
      var snapshotInfo = getConfFromSnapshot( db, hostName, svcName );
      checkResult( configs, snapshotInfo );
      var fileInfo = getConfFromFile( hostName, svcName );
      checkResult( configs, fileInfo );
   }

   // 删除多个run级别参数
   configs = getConfigs( "defaultVal" )["runConfigs"];
   deleteConf( db, configs, options );

   for( var i = 0; i < nodes.length; i++ )
   {
      var hostName = nodes[i].hostname;
      var svcName = nodes[i].svcname;
      var snapshotInfo = getConfFromSnapshot( db, hostName, svcName );
      checkResult( configs, snapshotInfo );
      var fileInfo = getConfFromFile( hostName, svcName );
      checkResult( configs, fileInfo, true );
   }

   // 更新多个reboot级别参数
   configs = getConfigs( "validVal" )["rebootConfigs"];
   var defaultVal = getConfigs( "defaultVal" )["rebootConfigs"];
   updateConf( db, configs, options, -322 );

   for( var i = 0; i < nodes.length; i++ )
   {
      var hostName = nodes[i].hostname;
      var svcName = nodes[i].svcname;
      var snapshotInfo = getConfFromSnapshot( db, hostName, svcName );
      checkResult( defaultVal, snapshotInfo );
      var fileInfo = getConfFromFile( hostName, svcName );
      checkResult( configs, fileInfo );
   }

   db.getRG( groupName ).stop();
   db.getRG( groupName ).start();

   for( var i = 0; i < nodes.length; i++ )
   {
      var hostName = nodes[i].hostname;
      var svcName = nodes[i].svcname;
      var snapshotInfo = getConfFromSnapshot( db, hostName, svcName );
      checkResult( configs, snapshotInfo );
      var fileInfo = getConfFromFile( hostName, svcName );
      checkResult( configs, fileInfo );
   }

   //删除多个reboot级别参数
   deleteConf( db, configs, options, -322 );

   for( var i = 0; i < nodes.length; i++ )
   {
      var hostName = nodes[i].hostname;
      var svcName = nodes[i].svcname;
      var snapshotInfo = getConfFromSnapshot( db, hostName, svcName );
      checkResult( configs, snapshotInfo );
      var fileInfo = getConfFromFile( hostName, svcName );
      checkResult( defaultVal, fileInfo, true );
   }

   db.getRG( groupName ).stop();
   db.getRG( groupName ).start();

   for( var i = 0; i < nodes.length; i++ )
   {
      var hostName = nodes[i].hostname;
      var svcName = nodes[i].svcname;
      var snapshotInfo = getConfFromSnapshot( db, hostName, svcName );
      checkResult( defaultVal, snapshotInfo );
      var fileInfo = getConfFromFile( hostName, svcName );
      checkResult( defaultVal, fileInfo, true );
   }

   // 更新多个forbid级别参数 
   var snapshotInfo_before = [];
   var fileInfo_before = [];
   for( var i = 0; i < nodes.length; i++ )
   {
      var hostName = nodes[i].hostname;
      var svcName = nodes[i].svcname;
      snapshotInfo_before.push( getConfFromSnapshot( db, hostName, svcName ) );
      fileInfo_before.push( getConfFromFile( hostName, svcName ) );
   }

   var configs = getConfigs( "validVal" )["forbidConfigs"];
   updateConf( db, configs, options, -322 );

   for( var i = 0; i < nodes.length; i++ )
   {
      var hostName = nodes[i].hostname;
      var svcName = nodes[i].svcname;
      var snapshotInfo_after = getConfFromSnapshot( db, hostName, svcName );
      checkResult( snapshotInfo_before[i], snapshotInfo_after );
      var fileInfo_after = getConfFromFile( hostName, svcName );
      checkResult( fileInfo_before[i], fileInfo_after );
   }

   // 删除多个forbid级别参数
   deleteConf( db, configs, options, -322 );

   for( var i = 0; i < nodes.length; i++ )
   {
      var hostName = nodes[i].hostname;
      var svcName = nodes[i].svcname;
      var snapshotInfo_after = getConfFromSnapshot( db, hostName, svcName );
      checkResult( snapshotInfo_before[i], snapshotInfo_after );
      var fileInfo_after = getConfFromFile( hostName, svcName );
      checkResult( fileInfo_before[i], fileInfo_after );
   }

   db.removeRG( groupName );
}
