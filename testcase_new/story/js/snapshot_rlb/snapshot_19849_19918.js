/******************************************************************************
 * @Description   : seqDB-19849:指定showError和showErrorMode查询节点健康快照 
 *                  seqDB-19918:指定showError和showErrorMode查询节点健康快照    
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.08.25
 * @LastEditTime  : 2022.10.21
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
main( test );

function test ()
{
   // 指定showError和showErrorMode查询节点健康快照(分别使用db.snapshot()、db.exec()方式)  
   var coordArr = getCoordUrl( db );
   if( coordArr.length < 2 )
   {
      return;
   }
   db = new Sdb( coordArr[0] );
   var coordRG = db.getCoordRG();
   var coord = coordRG.getNode( coordArr[1] );

   var cataRG = db.getCataRG();
   var cata = cataRG.getSlave();

   var groups = testPara.groups;
   var group = groups[0][0];
   var GroupName = group["GroupName"];
   var dataRG = db.getRG( GroupName );
   var data = dataRG.getSlave();

   var nodeAddresses = [
      { "hostName": coord.getHostName(), "svcName": coord.getServiceName() },
      { "hostName": cata.getHostName(), "svcName": cata.getServiceName() },
      { "hostName": data.getHostName(), "svcName": data.getServiceName() }
   ];

   coord.stop();
   cata.stop();
   data.stop();

   try
   {
      // 1)show、aggr显示节点错误信息
      var showError = "show";
      var showErrorMode = "aggr";
      var sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: showError, ShowErrorMode: showErrorMode } );
      var cursor = db.snapshot( SDB_SNAP_HEALTH, sdbsnapshotOption );
      showErrNodesInformation( cursor, nodeAddresses );

      var snapshotOption = "/*+use_option(ShowError, " + showError + ") use_option(ShowErrorMode, " + showErrorMode + ")*/";
      cursor = db.exec( "select * from $SNAPSHOT_HEALTH " + snapshotOption );
      showErrNodesInformation( cursor, nodeAddresses );

      // 2)show、flat显示节点错误信息 
      showError = "show";
      showErrorMode = "flat";
      sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: showError, ShowErrorMode: showErrorMode } );
      cursor = db.snapshot( SDB_SNAP_HEALTH, sdbsnapshotOption );
      showInformation( cursor, nodeAddresses );

      snapshotOption = "/*+use_option(ShowError, " + showError + ") use_option(ShowErrorMode, " + showErrorMode + ")*/";
      cursor = db.exec( "select * from $SNAPSHOT_HEALTH " + snapshotOption );
      showInformation( cursor, nodeAddresses );

      // 3)ignore、[aggr,flat]显示节点错误信息
      showError = "ignore";
      showErrorMode = ["aggr", "flat"];
      for( var i = 0; i < showErrorMode.length; i++ )
      {
         sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: showError, ShowErrorMode: showErrorMode[i] } );
         cursor = db.snapshot( SDB_SNAP_HEALTH, sdbsnapshotOption );
         var obj = cursor.current().toObj();
         var errNodes = obj["ErrNodes"];
         assert.equal( errNodes, undefined, JSON.stringify( obj ) );
      }
      cursor.close();

      for( var i = 0; i < showErrorMode.length; i++ )
      {
         snapshotOption = "/*+use_option(ShowError, " + showError + ") use_option(ShowErrorMode, " + showErrorMode[i] + ")*/";
         cursor = db.exec( "select * from $SNAPSHOT_HEALTH " + snapshotOption );
         var obj = cursor.current().toObj();
         var errNodes = obj["ErrNodes"];
         assert.equal( errNodes, undefined, JSON.stringify( obj ) );
      }
      cursor.close();

      // 5)only、flat显示节点错误信息
      showError = "only";
      showErrorMode = "flat";
      sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: showError, ShowErrorMode: showErrorMode } );
      cursor = db.snapshot( SDB_SNAP_HEALTH, sdbsnapshotOption );
      showInformation( cursor, nodeAddresses );

      snapshotOption = "/*+use_option(ShowError, " + showError + ") use_option(ShowErrorMode, " + showErrorMode + ")*/";
      cursor = db.exec( "select * from $SNAPSHOT_HEALTH " + snapshotOption );
      showInformation( cursor, nodeAddresses );
   }
   finally
   {
      coord.start();
      cata.start();
      data.start();
      commCheckBusinessStatus( db );
   }
}

function showErrNodesInformation ( cursor, nodeAddresses )
{
   var count = 0;
   var errNodes = cursor.current().toObj()["ErrNodes"];
   for( var i = 0; i < nodeAddresses.length; i++ )
   {
      var hostName = nodeAddresses[i]["hostName"];
      var svcName = nodeAddresses[i]["svcName"];
      for( var j = 0; j < errNodes.length; j++ )
      {
         var nodeName = errNodes[j]["NodeName"];
         var flag = errNodes[j]["Flag"];
         if( nodeName === hostName + ":" + svcName )
         {
            assert.equal( flag, SDB_NET_CANNOT_CONNECT, JSON.stringify( errNodes[j] ) );
            count++;
            break;
         }
      }
   }
   assert.equal( count, nodeAddresses.length );
   cursor.close();
}

function showInformation ( cursor, nodeAddresses )
{
   var count = 0;
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      var nodeName = obj["NodeName"];
      var flag = obj["Flag"];
      for( var i = 0; i < nodeAddresses.length; i++ )
      {
         var hostName = nodeAddresses[i]["hostName"];
         var svcName = nodeAddresses[i]["svcName"];
         if( nodeName === hostName + ":" + svcName )
         {
            assert.equal( flag, SDB_NET_CANNOT_CONNECT, JSON.stringify( obj ) );
            count++;
         }
      }
   }
   assert.equal( count, nodeAddresses.length );
   cursor.close();
}