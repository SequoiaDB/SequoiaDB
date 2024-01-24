/******************************************************************************
 * @Description   :  seqDB-28704:指定showError查询数据库快照，catalog节点异常
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.11.23
 * @LastEditTime  : 2022.11.24
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
main( test );

function test ()
{
   var cataRG = db.getCataRG();
   //获取catalog的备节点
   var cata = cataRG.getSlave();

   var nodeAddresses = [
      { "hostName": cata.getHostName(), "svcName": cata.getServiceName() }
   ];
   //停掉一个备节点
   cata.stop();

   try
   {
      //不匹配catalog节点，不显示catalog节点信息
      var showError = "show";
      var sdbsnapshotOption = new SdbSnapshotOption().options( { ShowError: showError } );
      var cursor = db.snapshot( SDB_SNAP_DATABASE, sdbsnapshotOption );
      var catalogNode = cursor.current().toObj()["ErrNodes"];
      errNode = [];
      assert.equal( catalogNode, errNode );

      // 1)show显示节点错误信息  
      showError = "show";
      sdbsnapshotOption = new SdbSnapshotOption().cond( { Role: "catalog" } ).options( { ShowError: showError } );
      cursor = db.snapshot( SDB_SNAP_DATABASE, sdbsnapshotOption );
      showErrNodesInformation( cursor, nodeAddresses );

      // 2)ignore显示节点错误信息
      showError = "ignore";
      sdbsnapshotOption = new SdbSnapshotOption().cond( { Role: "catalog" } ).options( { ShowError: showError } );
      cursor = db.snapshot( SDB_SNAP_DATABASE, sdbsnapshotOption );
      errNodes = cursor.current().toObj()["ErrNodes"];
      assert.equal( errNodes, undefined, "showError指定为ignore,不显示错误信息" );

      cursor.close();

      // 3)only显示节点错误信息
      showError = "only";
      sdbsnapshotOption = new SdbSnapshotOption().cond( { Role: "catalog" } ).options( { ShowError: showError } );
      cursor = db.snapshot( SDB_SNAP_DATABASE, sdbsnapshotOption );
      showErrNodesInformation( cursor, nodeAddresses );
   }
   finally
   {
      cata.start();
      commCheckBusinessStatus( db );
   }
}

function showErrNodesInformation ( cursor, nodeAddresses )
{
   var count = 0;
   // 获取游标返回的ErrNodes
   var errNodes = cursor.current().toObj()["ErrNodes"];
   for( var i = 0; i < nodeAddresses.length; i++ )
   {
      var hostName = nodeAddresses[i]["hostName"];
      var svcName = nodeAddresses[i]["svcName"];
      for( var j = 0; j < errNodes.length; j++ )
      {
         // 获取返回ErrNodes中的NodeName
         var nodeName = errNodes[j]["NodeName"];
         // 获取返回ErrNodes中的Flag(错误码)
         var flag = errNodes[j]["Flag"];
         if( nodeName === hostName + ":" + svcName )
         {
            assert.equal( flag, SDB_NET_CANNOT_CONNECT, "停节点的节点名与节点错误信息中的节点名相同" );
            count++;
            break;
         }
      }
   }
   assert.equal( count, nodeAddresses.length, "停节点的个数与节点错误信息中节点个数相同" );
   cursor.close();
}