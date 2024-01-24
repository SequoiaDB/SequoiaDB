/******************************************************************************
 * @Description   : seqDB-32328:恢复主表删除lob
 * @Author        : liuli
 * @CreateTime    : 2023.07.12
 * @LastEditTime  : 2023.07.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var mainCLName = "mainCL_32328";
   var subCLName1 = "subCL_32328_1_" + generateRandomString( 5 );
   var subCLName2 = "subCL_32328_2_" + generateRandomString( 5 );

   var filePath = WORKDIR + "/lob32328/";
   var fileName = "filelob_32328";
   var fileSize = 1024 * 1024;

   commDropCL( db, COMMCSNAME, mainCLName );

   var shardingFormat = "YYYYMMDD";
   var beginBound = new Date().getFullYear() * 10000 + 101;
   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": shardingFormat, "ShardingType": "range" };
   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, options );
   commCreateCL( db, COMMCSNAME, subCLName1, { "ShardingKey": { "date": 1 }, "ShardingType": "hash" } );
   commCreateCL( db, COMMCSNAME, subCLName2, { "ShardingKey": { "date": 1 }, "ShardingType": "hash" } );
   var lowBound = { "date": ( parseInt( beginBound ) ) + '' };
   var upBound = { "date": ( parseInt( beginBound ) + 5 ) + '' };
   mainCL.attachCL( COMMCSNAME + "." + subCLName1, { "LowBound": lowBound, "UpBound": upBound } );
   var lowBound = { "date": ( parseInt( beginBound ) + 5 ) + '' };
   var upBound = { "date": ( parseInt( beginBound ) + 10 ) + '' };
   mainCL.attachCL( COMMCSNAME + "." + subCLName2, { "LowBound": lowBound, "UpBound": upBound } );

   // 插入lob
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobOids = insertLob( mainCL, filePath + fileName );

   // 删除lob
   mainCL.deleteLob( lobOids[0] );
   mainCL.deleteLob( lobOids[7] );

   // 获取子表sub1所在的group
   var groupNames = commGetCLGroups( db, COMMCSNAME + "." + subCLName1 );

   db.sync( { GroupName: groupNames[0] } );

   // 获取主节点
   var masterNode = db.getRG( groupNames[0] ).getMaster();
   var hostName = masterNode.getHostName();
   var scvName = masterNode.getServiceName();
   var masterNodeName = hostName + ":" + scvName;

   // 获取同步日志路径
   var replicalogPath = getReplicalogPath( db, masterNodeName );

   var remoteObj = new Remote( hostName, CMSVCNAME );
   var cmd = remoteObj.getCmd();

   // 创建临时集合用于恢复数据
   var clNameTmp = "cl_32328_tmp";
   var tmpCL = commCreateCL( db, COMMCSNAME, clNameTmp, {}, true, true );

   // 指定主表恢复数据
   var installDir = commGetRemoteInstallPath( hostName, CMSVCNAME );
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + mainCLName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname "
      + COORDHOSTNAME + " --svcname " + COORDSVCNAME;

   cmd.run( command );

   var cursor = tmpCL.listLobs();
   if( cursor.next() )
   {
      throw new Error( "expect cursor to be empty,cusror : " + JSON.stringify( cursor.current().toObj() ) );
   }
   cursor.close();

   // 指定子表恢复数据
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + subCLName1 + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname "
      + COORDHOSTNAME + " --svcname " + COORDSVCNAME;
   cmd.run( command );

   // 校验恢复的lob
   tmpCL.getLob( lobOids[0], filePath + "checkputlob32328", true );
   var actMD5 = File.md5( filePath + "checkputlob32328" );
   assert.equal( fileMD5, actMD5 );

   // 另一个子表中删除的lob不会恢复
   assert.tryThrow( SDB_FNE, function()
   {
      tmpCL.getLob( lobOids[7], filePath + "checkputlob32328", true );
   } );

   remoteObj.close();
   commDropCL( db, COMMCSNAME, clNameTmp );
   commDropCL( db, COMMCSNAME, mainCLName );
}

function insertLob ( mainCL, filePath )
{
   var year = new Date().getFullYear();
   var month = 1;
   var day = 1;
   var lobOids = [];

   for( var j = 0; j < 10; j++ )
   {
      var timestamp = year + "-" + month + "-" + ( day + j ) + "-00.00.00.000000";
      var lobOid = mainCL.createLobID( timestamp );
      var lobOid = mainCL.putLob( filePath, lobOid );
      lobOids.push( lobOid );
   }
   return lobOids;
}
