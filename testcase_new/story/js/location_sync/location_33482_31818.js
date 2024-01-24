/******************************************************************************
 * @Description   : seqDB-33482:coord节点启动运维模式
 *                : seqDB-31818:不支持启动Critical模式的节点启动Critical模式
 * @Author        : liuli
 * @CreateTime    : 2023.09.21
 * @LastEditTime  : 2023.10.09
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location = "location_33482_31818";
   var coordUrls = getCoordUrl( db );

   // 指定coord节点启动运维模式
   var coordRG = db.getCoordRG();
   var options = { NodeName: coordUrls[0], MinKeepTime: 10, MaxKeepTime: 20 };
   assert.tryThrow( SDB_OPERATION_CONFLICT, function()
   {
      coordRG.startMaintenanceMode( options );
   } )

   // 指定coord节点启动Critical模式
   assert.tryThrow( SDB_OPERATION_CONFLICT, function()
   {
      coordRG.startCriticalMode( options );
   } )

   // coord节点设置Location
   db.getCoordRG().getNode( coordUrls[0] ).setLocation( location );

   // 指定coord节点所在Location启动运维模式
   options = { Location: location, MinKeepTime: 10, MaxKeepTime: 20 };
   assert.tryThrow( SDB_OPERATION_CONFLICT, function()
   {
      coordRG.startMaintenanceMode( options );
   } )

   // 指定coord节点所在Location启动Critical模式
   assert.tryThrow( SDB_OPERATION_CONFLICT, function()
   {
      coordRG.startCriticalMode( options );
   } )

   db.getCoordRG().getNode( coordUrls[0] ).setLocation( "" );
}

function getCoordUrl ( sdb )
{
   var coordUrls = [];
   var rgInfo = sdb.getCoordRG().getDetail().current().toObj().Group;
   for( var i = 0; i < rgInfo.length; i++ )
   {
      var hostname = rgInfo[i].HostName;
      var svcname = rgInfo[i].Service[0].Name;
      coordUrls.push( hostname + ":" + svcname );
   }
   return coordUrls;
}