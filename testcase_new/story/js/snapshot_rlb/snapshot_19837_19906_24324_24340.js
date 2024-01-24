/******************************************************************************
 * @Description   : seqDB-19837:指定showError和showErrorMode查询数据库快照
 *                : seqDB-19906:指定showError和showErrorMode查询数据库快照  
 *                : seqDB-24324:指定showError查询数据库快照，不存在异常节点
 *                : seqDB-24340:SdbSnapshotOption接口指定options为空     
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.08.18
 * @LastEditTime  : 2022.10.21
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
main( test );

function test ()
{
   // 1.指定showError查询数据库快照,不存在异常节点
   // 1)show显示节点错误信息
   var sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: "show" } );
   var cursor = db.snapshot( SDB_SNAP_DATABASE, sdbsnapshotOption );
   var errNodes = cursor.current().toObj()["ErrNodes"];
   assert.equal( errNodes, [] );
   // 2)ignore显示节点错误信息
   var sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: "ignore" } );
   var cursor = db.snapshot( SDB_SNAP_DATABASE, sdbsnapshotOption );
   var errNodes = cursor.current().toObj()["ErrNodes"];
   assert.equal( errNodes, undefined );
   // 3)only显示节点错误信息
   var sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: "only" } );
   var cursor = db.snapshot( SDB_SNAP_DATABASE, sdbsnapshotOption );
   while( cursor.next() )
   {
      throw new Error( "should error but success\n" + JSON.stringify( cursor.current().toObj() ) );
   }
   cursor.close();

   // 2.指定SdbSnapshotOption接口为空查询快照
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.snapshot( SDB_SNAP_CONFIGS, new SdbSnapshotOption().options() );
   } )

   // 3.指定showError和showErrorMode查询数据库快照,存在异常节点(分别使用db.snapshot()、db.exec()方式) 
   var coordArr = getCoordUrl( db );
   if( coordArr.length < 2 )
   {
      return;
   }
   db = new Sdb( coordArr[0] );
   var coordRG = db.getCoordRG();
   var coord = coordRG.getNode( coordArr[1] );

   var groups = testPara.groups;
   var group = groups[0][0];
   var GroupName = group["GroupName"];
   var dataRG = db.getRG( GroupName );
   var data = dataRG.getSlave();

   var nodeAddresses = [
      { "hostName": coord.getHostName(), "svcName": coord.getServiceName() },
      { "hostName": data.getHostName(), "svcName": data.getServiceName() }
   ];

   coord.stop();
   data.stop();

   try
   {
      // 1)show、aggr显示节点错误信息  
      var showError = "show";
      var showErrorMode = "aggr";
      var sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: showError, ShowErrorMode: showErrorMode } );
      var cursor = db.snapshot( SDB_SNAP_DATABASE, sdbsnapshotOption );
      showErrNodesInformation( cursor, nodeAddresses );

      var snapshotOption = "/*+use_option(ShowError, " + showError + ") use_option(ShowErrorMode, " + showErrorMode + ")*/";
      cursor = db.exec( "select * from $SNAPSHOT_DB " + snapshotOption );
      showErrNodesInformation( cursor, nodeAddresses );

      // 2)show、flat显示节点错误信息 
      showError = "show";
      showErrorMode = "flat";
      sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: showError, ShowErrorMode: showErrorMode } );
      cursor = db.snapshot( SDB_SNAP_DATABASE, sdbsnapshotOption );
      showErrNodesInformation( cursor, nodeAddresses );

      snapshotOption = "/*+use_option(ShowError, " + showError + ") use_option(ShowErrorMode, " + showErrorMode + ")*/";
      cursor = db.exec( "select * from $SNAPSHOT_DB " + snapshotOption );
      showInformation( cursor, nodeAddresses );

      // 3)ignore、[aggr,flat]显示节点错误信息
      showError = "ignore";
      showErrorMode = ["aggr", "flat"];
      for( var i = 0; i < showErrorMode.length; i++ )
      {
         sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: showError, ShowErrorMode: showErrorMode[i] } );
         cursor = db.snapshot( SDB_SNAP_DATABASE, sdbsnapshotOption );
         var obj = cursor.current().toObj();
         var errNodes = obj["ErrNodes"];
         assert.equal( errNodes, undefined, JSON.stringify( obj ) );
      }
      cursor.close();

      for( var i = 0; i < showErrorMode.length; i++ )
      {
         snapshotOption = "/*+use_option(ShowError, " + showError + ") use_option(ShowErrorMode, " + showErrorMode[i] + ")*/";
         cursor = db.exec( "select * from $SNAPSHOT_DB " + snapshotOption );
         var obj = cursor.current().toObj();
         var errNodes = obj["ErrNodes"];
         assert.equal( errNodes, undefined, JSON.stringify( obj ) );
      }
      cursor.close();

      // 4)only、aggr显示节点错误信息
      showError = "only";
      showErrorMode = "aggr";
      sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: showError, ShowErrorMode: showErrorMode } );
      cursor = db.snapshot( SDB_SNAP_DATABASE, sdbsnapshotOption );
      showErrNodesInformation( cursor, nodeAddresses );

      snapshotOption = "/*+use_option(ShowError, " + showError + ") use_option(ShowErrorMode, " + showErrorMode + ")*/";
      cursor = db.exec( "select * from $SNAPSHOT_DB " + snapshotOption );
      showErrNodesInformation( cursor, nodeAddresses );

      // 5)only、flat显示节点错误信息
      showError = "only";
      showErrorMode = "flat";
      sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: showError, ShowErrorMode: showErrorMode } );
      cursor = db.snapshot( SDB_SNAP_DATABASE, sdbsnapshotOption );
      showInformation( cursor, nodeAddresses );

      snapshotOption = "/*+use_option(ShowError, " + showError + ") use_option(ShowErrorMode, " + showErrorMode + ")*/";
      cursor = db.exec( "select * from $SNAPSHOT_DB " + snapshotOption );
      showInformation( cursor, nodeAddresses );
   } finally
   {
      coord.start();
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