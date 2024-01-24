/******************************************************************************
 * @Description   : seqDB-31839:通过dataCenter删除ActiveLocation
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.26
 * @LastEditTime  : 2023.06.16
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );
function test ()
{
   var location = "location_31839";

   var rg = db.getCoordRG();
   var coord = getGroupSlaveNodeName( db, COORD_GROUPNAME );
   var coordNode = rg.getNode( coord[0] );
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
   // 不存在ActiveLocation，执行删除
   dc.setActiveLocation( "" );
   checkGroupActiveLocation( db, groups, "" );

   // 设置ActiveLocation
   dc.setActiveLocation( location );
   // 删除ActiveLocation
   dc.setActiveLocation( "" );
   checkGroupActiveLocation( db, groups, "" );

   // group2、group3不存在ActiveLocation，group1存在ActiveLocation执行删除
   var rg = db.getRG( groups[0] );
   rg.setActiveLocation( location );
   dc.setActiveLocation( "" );
   checkGroupActiveLocation( db, groups, "" );

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