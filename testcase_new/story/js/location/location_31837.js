/******************************************************************************
 * @Description   : seqDB-31837:通过dataCenter设置ActiveLocation
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.26
 * @LastEditTime  : 2023.06.09
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location = "location_31837";
   var rg = db.getCoordRG();
   var coord = getGroupSlaveNodeName( db, COORD_GROUPNAME );
   var coordNode = rg.getNode( coord[0] );
   var hostName = coordNode.getHostName();
   coordNode.setLocation( location );

   var catalog = db.getCatalogRG().getMaster();
   catalog.setLocation( location );

   var groups = commGetDataGroupNames( db );
   for( var i = 0; i < groups.length; i++ )
   {
      var rg = db.getRG( groups[i] );
      var node = rg.getMaster();
      node.setLocation( location );
   }

   var dc = db.getDC();
   dc.setActiveLocation( location );
   checkGroupActiveLocation( db, groups, location );

   // 删除coord的location
   coordNode.setLocation( "" );
   dc.setActiveLocation( location );
   checkGroupActiveLocation( db, groups, location );

   // 删除cata的location
   catalog.setLocation( "" );
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      dc.setActiveLocation( location );
   } );

   // 删除部分data组的location
   node.setLocation( "" );
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      dc.setActiveLocation( location );
   } );

   // 清除所有节点的location
   coordNode.setLocation( "" );
   catalog.setLocation( "" );
   for( var i = 0; i < groups.length; i++ )
   {
      var rg = db.getRG( groups[i] );
      var node = rg.getMaster();
      node.setLocation( "" );
   }
}