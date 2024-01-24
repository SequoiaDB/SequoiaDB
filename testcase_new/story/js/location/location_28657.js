/******************************************************************************
 * @Description   : seqDB-28657:使用setAttributes设置option中的location参数校验
 * @Author        : liuli
 * @CreateTime    : 2022.11.14
 * @LastEditTime  : 2022.11.30
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   // 获取一个data主节点
   var dataGroupName = commGetDataGroupNames( db )[0];
   var data = db.getRG( dataGroupName ).getMaster();
   data.setLocation( "" );

   // 设置节点location为非法值
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      data.setAttributes( { Location: 1 } );
   } );
   checkNodeLocation( data, undefined );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      data.setAttributes( { Location: true } );
   } );
   checkNodeLocation( data, undefined );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      data.setAttributes( { Location: null } );
   } );
   checkNodeLocation( data, undefined );

   // 设置节点location大于256个字符
   var arr = new Array( 258 );
   var location = arr.join( "a" );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      data.setAttributes( { Location: location } );
   } );
   checkNodeLocation( data, undefined );

   // 设置中文location
   var location = "巨杉数据库";
   data.setAttributes( { Location: location } );
   checkNodeLocation( data, location );

   // 设置中英文混合location
   var location = "巨杉数据库SequoiaDB";
   data.setAttributes( { Location: location } );
   checkNodeLocation( data, location );

   // 随机生成location
   for( var i = 0; i < 10; i++ )
   {
      var location = '';
      for( var j = 0; j < 10; j++ )
      {
         location += String.fromCharCode( ( Math.floor( Math.random() * 94 ) + 33 ) );
      }
      data.setAttributes( { Location: location } );
      checkNodeLocation( data, location );
   }

   // 清理location
   data.setLocation( "" );
}
