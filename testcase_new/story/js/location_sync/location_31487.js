/******************************************************************************
 * @Description   : seqDB-31487: reelectLocation()接口中的seconds参数校验
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.11
 * @LastEditTime  : 2023.06.07
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location = "location_31487";

   var group = commGetDataGroupNames( db )[0];
   var rg = db.getRG( group );
   var nodelist = commGetGroupNodes( db, group );

   setLocationForNodes( rg, nodelist, location );
   var masterNode = rg.getMaster();
   masterNode.setLocation( "" );
   checkAndGetLocationHasPrimary( db, group, location, 10 );

   // 指定Seconds小于10秒
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( location, { Seconds: 5 } );
   } );

   // 指定Seconds为非法参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( location, { Seconds: "location" } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( location, { Seconds: true } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( location, { Seconds: null } );
   } );

   // 设置为负数报错
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( location, { Seconds: -1 } );
   } );

   setLocationForNodes( rg, nodelist, "" );
}