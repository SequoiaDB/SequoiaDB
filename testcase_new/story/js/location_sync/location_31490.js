/******************************************************************************
 * @Description   : seqDB-31490: reelectLocation()接口中的serviceName参数校验
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
   var location = "location_31490";

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

   // 指定复制组中存在的ServiceName
   rg.reelectLocation( location, { ServiceName: group[1].svcname } );

   // 设置不存在的ServiceName
   assert.tryThrow( SDB_CLS_NODE_NOT_EXIST, function()
   {
      rg.reelectLocation( location, { ServiceName: "3000000" } );
   } );

   // 指定ServiceName为非法参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( location, { ServiceName: 1111 } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( location, { ServiceName: true } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( location, { ServiceName: null } );
   } );

   setLocationForNodes( rg, nodelist, "" );
}