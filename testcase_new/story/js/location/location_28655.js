/******************************************************************************
 * @Description   : seqDB-28655:使用setLocation设置Location参数校验
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.11.15
 * @LastEditTime  : 2022.12.01
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   // 获取一个catalog节点
   var catalog = db.getCatalogRG().getSlave();

   // 设置节点location为非法值
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      catalog.setLocation( 1 );
   } );
   checkNodeLocation( catalog, undefined );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      catalog.setLocation( true );
   } );
   checkNodeLocation( catalog, undefined );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      catalog.setLocation( null );
   } );
   checkNodeLocation( catalog, undefined );

   // 使用setLocation不设置值
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      catalog.setLocation();
   } );
   checkNodeLocation( catalog, undefined );

   // 设置节点location大于256个字符
   var arr = new Array( 258 );
   var location = arr.join( "a" );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      catalog.setLocation( location );
   } );
   checkNodeLocation( catalog, undefined );

   // 设置中文location
   var location = "巨杉数据库";
   catalog.setLocation( location );
   checkNodeLocation( catalog, location );

   // 设置中英文混合location
   var location = "巨杉数据库SequoiaDB";
   catalog.setLocation( location );
   checkNodeLocation( catalog, location );

   // 随机生成location
   for( var i = 0; i < 10; i++ )
   {
      var location = '';
      for( var j = 0; j < 10; j++ )
      {
         location += String.fromCharCode( ( Math.floor( Math.random() * 94 ) + 33 ) );
      }
      catalog.setAttributes( { Location: location } );
      checkNodeLocation( catalog, location );
   }

   // 清理location
   catalog.setLocation( "" );
}