/******************************************************************************
 * @Description   : seqDB-31488: reelectLocation()接口中的NodeID参数校验
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.11
 * @LastEditTime  : 2023.06.12
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location = "location_31488";

   var group = commGetDataGroupNames( db )[0];
   var rg = db.getRG( group );
   var nodelist = commGetGroupNodes( db, group );

   //清空节点location
   setLocationForNodes( rg, nodelist, "" );

   // 将复制组1中的所有节点设置location
   setLocationForNodes( rg, nodelist, location );
   // 将主节点的location设置为""
   var masterNode = rg.getMaster();
   masterNode.setLocation( "" );
   checkAndGetLocationHasPrimary( db, group, location, 10 );

   // 指定复制组中存在的NodeID
   rg.reelectLocation( location, { NodeID: group[1].NodeID } );

   // 设置不存在的NodeID
   assert.tryThrow( SDB_CLS_NODE_NOT_EXIST, function()
   {
      rg.reelectLocation( location, { NodeID: 1000000 } );
   } );

   // 指定NodeID为非法参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( location, { NodeID: "location" } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( location, { NodeID: true } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( location, { NodeID: null } );
   } );

   setLocationForNodes( rg, nodelist, "" );
}