testConf.skipStandAlone = true;
main( test );

function test ()
{
   var groupName = "group20124";
   var instanceidList = [0, 0, 0, 100, 150, 200];
   var nodeNum = 6;

   // 创建三个节点不指定instanceid，创建三个数据结点并指定instanceid
   var nodeInfos = mycommCreateRG( db, groupName, instanceidList );

   //指定不存在的InstanceID
   var matcher = { InstanceID: 250, RawData: true, GroupName: groupName };
   var expResult = [];
   checkInstanceID( db, matcher, expResult, -155 );

   var matcher = { instanceid: 250, RawData: true, GroupName: groupName };
   var expResult = [];
   checkInstanceID( db, matcher, expResult, -155 );

   //指定存在的InstanceID
   var matcher = { InstanceID: 100, RawData: true, GroupName: groupName };
   var expResult = [nodeInfos[3]];
   checkInstanceID( db, matcher, expResult );

   var matcher = { instanceid: 100, RawData: true, GroupName: groupName };
   var expResult = [nodeInfos[3]];
   checkInstanceID( db, matcher, expResult );

   //指定多个存在的InstanceID
   var matcher = { InstanceID: [100, 200], RawData: true, GroupName: groupName };
   var expResult = [nodeInfos[3], nodeInfos[5]];
   checkInstanceID( db, matcher, expResult );

   var matcher = { instanceid: [100, 150], RawData: true, GroupName: groupName };
   var expResult = [nodeInfos[3], nodeInfos[4]];
   checkInstanceID( db, matcher, expResult );

   var matcher = { InstanceID: 150, instanceid: 200, RawData: true, GroupName: groupName };
   var expResult = [nodeInfos[4], nodeInfos[5]];
   checkInstanceID( db, matcher, expResult );

   //指定多个InstanceID，一个存在一个不存在
   var matcher = { InstanceID: [200, 250], RawData: true, GroupName: groupName };
   var expResult = [nodeInfos[5]];
   checkInstanceID( db, matcher, expResult );

   var matcher = { instanceid: [200, 250], RawData: true, GroupName: groupName };
   var expResult = [nodeInfos[5]];
   checkInstanceID( db, matcher, expResult );

   var matcher = { InstanceID: 250, instanceid: 200, RawData: true, GroupName: groupName };
   var expResult = [nodeInfos[5]];
   checkInstanceID( db, matcher, expResult );

   //指定InstanceID排在第一位的节点？若第一位的instanceid=1,则优先返回
   var matcher = { InstanceID: 1, RawData: true, GroupName: groupName };
   var expResult = [nodeInfos[0]];
   checkInstanceID( db, matcher, expResult );

   //指定InstanceID排在第一位和第二位的节点
   var matcher = { InstanceID: 1, instanceid: 2, RawData: true, GroupName: groupName };
   var expResult = [nodeInfos[0], nodeInfos[1]];
   checkInstanceID( db, matcher, expResult );

   //指定InstanceID排在第一位和单个InstanceID节点
   var matcher = { InstanceID: 100, instanceid: 2, RawData: true, GroupName: groupName };
   var expResult = [nodeInfos[1], nodeInfos[3]];
   checkInstanceID( db, matcher, expResult );

   //数组和单个的情况
   var matcher = { InstanceID: [200, 100], instanceid: 2, RawData: true, GroupName: groupName };
   var expResult = [nodeInfos[1], nodeInfos[3], nodeInfos[5]];
   checkInstanceID( db, matcher, expResult );

   db.removeRG( "group20124" );
}

function mycommCreateRG ( db, rgName, instanceidList )
{

   var nodeList = commGetSnapshot( db, SDB_SNAP_SYSTEM, { Role: "coord", RawData: true } );
   hostname = nodeList[0].HostName;
   var rg = db.createRG( rgName );
   var maxRetryTimes = 100;
   var nodeInfos = [];
   for( var i = 0; i < instanceidList.length; i++ )
   {
      var failedCount = 0;
      var svc = parseInt( RSRVPORTBEGIN ) + 10 * ( i + failedCount );
      var dbPath = RSRVNODEDIR + "data/" + svc;
      do
      {
         try
         {
            new Remote( hostname ).getCmd().run( "lsof -i:" + svc );
            svc = svc + 10;
            dbPath = RSRVNODEDIR + "data/" + svc;
            failedCount++;
            continue;
         }
         catch( e )
         {
            if( e.message != 1 )
            {
               throw new Error( "lsof check port error: " + e );
            }
         }
         try
         {
            var nodeOption = {};
            nodeOption.diglevel = 5;
            if( instanceidList[i] != 0 )
            {
               nodeOption.instanceid = instanceidList[i];
            }
            rg.createNode( hostname, svc, dbPath, nodeOption );
            println( "create node: " + hostname + ":" + svc + " dbpath: " + dbPath );
            var nodeInfo = { "HostName": hostname, "ServiceName": JSON.stringify( svc ) };
            nodeInfos.push( nodeInfo );
            break;
         }
         catch( e )
         {
            //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
            if( commCompareErrorCode( e, SDBCM_NODE_EXISTED ) || commCompareErrorCode( e, SDB_DIR_NOT_EMPTY ) )
            {
               svc = svc + 10;
               dbPath = RSRVNODEDIR + "data/" + svc;
               failedCount++;
            }
            else
            {
               throw new Error( "create node failed!  port = " + svc + " dataPath = " + dbPath + " errorCode: " + e );
            }
         }
      }
      while( failedCount < maxRetryTimes );
   }
   rg.start();
   return nodeInfos;
}

function checkInstanceID ( db, matcher, expResult, errno )
{
   if( errno !== undefined )
   {
      try
      {
         db.snapshot( SDB_SNAP_DATABASE, matcher );
         throw new Error( "catch no errno !" );
      }
      catch( e )
      {
         if( e.message != errno )
         {
            throw new Error( "check matcher " + JSON.stringify( matcher ) + ", \nexp: " + errno + ", \nact: " + e.message );
         }
      }
   }
   else
   {
      var cursor = db.snapshot( SDB_SNAP_DATABASE, matcher, { "HostName": "", "ServiceName": "" }, { "svcname": 1 } );
      commCompareResults( cursor, expResult );
   }
}
