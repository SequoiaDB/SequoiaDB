/******************************************************************************
 * @Description :  seqDB-18075:多次执行updateConf更新不同配置项
 * @author      : luweikang
 * @date        ：2019.04.04
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var nodeNum = 1;
   var groupName = "rg_18075";
   var hostName = commGetGroups( db )[0][1].HostName;
   var nodeOption = { diaglevel: 3 };
   var nodes = commCreateRG( db, groupName, nodeNum, hostName, nodeOption );

   //指定diaglevel创建节点会将此参数写入节点conf文件，由于后面会校验节点conf文件里的配置参数，将此参数从conf文件里删除，以便后面校验
   var config = { "diaglevel": 1 };
   var options = { "HostName": nodes[0].hostname, "ServiceName": nodes[0].svcname.toString() };
   deleteConf( db, config, options );

   configs = { "transactionon": "FALSE" };
   updateConf( db, configs, options, -322 );
   check( nodes, configs );

   configs["numpreload"] = 10;
   updateConf( db, configs, options, -322 );
   check( nodes, configs );

   configs["diaglevel"] = 5;
   updateConf( db, configs, options, -322 );
   check( nodes, configs );

   configs["logfilesz"] = 10;
   updateConf( db, configs, options, -322 );
   delete configs.logfilesz;
   check( nodes, configs );

   db.getRG( groupName ).stop();
   db.getRG( groupName ).start();

   var snapshotInfo = getConfFromSnapshot( db, nodes[0].hostname, nodes[0].svcname );
   checkResult( configs, snapshotInfo );
   var fileInfo = getConfFromFile( nodes[0].hostname, nodes[0].svcname );
   checkResult( configs, fileInfo );

   db.removeRG( groupName );
}

function check ( nodes, configs )
{
   var nodeName = nodes[0].hostname + ":" + nodes[0].svcname;
   var sdbSnapshotOption = new SdbSnapshotOption().cond( { NodeName: nodeName } ).options( { "mode": "local", "expand": false } );
   var snapshotInfo = db.snapshot( SDB_SNAP_CONFIGS, sdbSnapshotOption ).next().toObj();
   for( var key in configs )
   {
      if( configs[key].toString().toUpperCase() !== snapshotInfo[key].toString().toUpperCase() )
      {
         throw new Error( "The expected result is " + configs[key].toString().toUpperCase() + ", but the actual resutl is " +
            snapshotInfo[key].toString().toUpperCase() );
      }
   }
}
