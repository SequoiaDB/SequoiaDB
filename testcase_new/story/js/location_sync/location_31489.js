/******************************************************************************
 * @Description   : seqDB-31489: reelectLocation()接口中的hostName参数校验
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.22
 * @LastEditTime  : 2023.06.12
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location = "location_31489";

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

   // 指定复制组中存在的HostName
   rg.reelectLocation( location, { HostName: group[1].HostName } );

   // 设置不存在的HostName
   assert.tryThrow( SDB_CLS_NODE_NOT_EXIST, function()
   {
      rg.reelectLocation( location, { HostName: "nodehostname-31489" } );
   } );

   // 指定HostName为非法参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( location, { HostName: 11111 } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( location, { HostName: true } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( location, { HostName: null } );
   } );

   setLocationForNodes( rg, nodelist, "" );
}