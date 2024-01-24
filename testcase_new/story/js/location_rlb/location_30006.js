/******************************************************************************
 * @Description   : seqDB-30006:复制组节点与位置集主节点相同/不相同时，使用reelectLocation方法
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.02.06
 * @LastEditTime  : 2023.10.07
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location = "location_30006";

   var dataGroupName = commGetDataGroupNames( db )[0];
   var rg = db.getRG( dataGroupName );
   var group = commGetGroups( db, true, dataGroupName, true, false, true )[0];

   for( var i = 1; i < group.length; i++ )
   {
      var groupInfo = group[i];
      var hostName = groupInfo.HostName;
      var svcName = groupInfo.svcname;
      var data = rg.getNode( hostName, svcName );
      data.setAttributes( { Location: location } );
   }

   // 检查复制组主节点与location主节点是否一致
   checkAndGetLocationHasPrimary( db, dataGroupName, location, 60 );
   var cursor = db.snapshot( SDB_SNAP_SYSTEM, { RawData: true, GroupName: dataGroupName, IsPrimary: true } );
   while( cursor.next() )
   {
      var IsLocationPrimary = cursor.current().toObj().IsLocationPrimary;
      var NodeID = cursor.current().toObj().NodeID;
      var nodeId = NodeID[1];
      if( IsLocationPrimary != true )
      {
         rg.reelectLocation( location, { NodeID: nodeId } );
      }
   }
   assert.tryThrow( SDB_OPERATION_CONFLICT, function()
   {
      rg.reelectLocation( location, { NodeID: nodeId } );
   } );

   cursor.close();
}