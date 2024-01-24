/******************************************************************************
 * @Description   : seqDB-31486: reelectLocation()接口中的location参数校验
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.11
 * @LastEditTime  : 2023.06.02
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location1 = "location_31486_1";
   var location2 = "location_31486_2";
   // 获取一个data节点
   var group = commGetDataGroupNames( db )[0];
   var rg = db.getRG( group );
   var data = rg.getSlave();

   // 设置location
   data.setLocation( location1 );
   checkAndGetLocationHasPrimary( db, group, location1, 10 );

   // 指定不存在的location
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( location2 );
   } );

   // location指定非法参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( 1 );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( true );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      rg.reelectLocation( null );
   } );

   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      rg.reelectLocation();
   } );

   data.setLocation( "" );
}