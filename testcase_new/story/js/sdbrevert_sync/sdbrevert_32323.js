/******************************************************************************
 * @Description   : seqDB-32323:主表删除数据进行恢复
 * @Author        : liuli
 * @CreateTime    : 2023.07.12
 * @LastEditTime  : 2023.07.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var mainCLName = "mainCL_32323";
   var subCLName1 = "subCL_32323_1_" + generateRandomString( 5 );
   var subCLName2 = "subCL_32323_2_" + generateRandomString( 5 );

   commDropCL( db, COMMCSNAME, mainCLName );

   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   // 插入数据
   var docs = [];
   for( var i = 0; i < 2000; i++ )
   {
      docs.push( { _id: i, a: i, b: i } );
   }
   mainCL.insert( docs );

   // 删除数据
   mainCL.remove( { a: { $gt: 500, $lt: 1500 } } );

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
   var clNameTmp = "cl_32322_tmp";
   var tmpCL = commCreateCL( db, COMMCSNAME, clNameTmp, {}, true, true );

   // 指定主表恢复数据
   var installDir = commGetRemoteInstallPath( hostName, CMSVCNAME );
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + mainCLName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname "
      + COORDHOSTNAME + " --svcname " + COORDSVCNAME;

   cmd.run( command );

   var expResult = [];
   var actResult = tmpCL.find();
   commCompareResults( actResult, expResult );

   // 指定子表恢复数据
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + subCLName1 + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname "
      + COORDHOSTNAME + " --svcname " + COORDSVCNAME;
   cmd.run( command );
   for( var i = 501; i < 1000; i++ )
   {
      expResult.push( { entry: { _id: i, a: i, b: i }, optype: "DOC_DELETE", "label": "" } );
   }
   var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } ).sort( { "entry.a": 1 } );
   commCompareResults( actResult, expResult );

   remoteObj.close();
   commDropCL( db, COMMCSNAME, clNameTmp );
   commDropCL( db, COMMCSNAME, mainCLName );
}