/******************************************************************************
 * @Description   : seqDB-31846:SdbDC.setActiveLocation(<location>)参数校验
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.26
 * @LastEditTime  : 2023.10.08
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var dc = db.getDC();
   var node = db.getRG( "SYSCatalogGroup" ).getSlave();
   var hostName = node.getHostName();

   // 类型为非string
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dc.setActiveLocation( 123 );
   } );

   var text = "";
   for( var i = 0; i < 257; i++ )
   {
      text += "a";
   }
   // 类型长度超出256
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dc.setActiveLocation( text );
   } );

   // 类型长度超出255
   var location = "";
   for( var i = 0; i < 255; i++ )
   {
      location += "b";
   }
   dc.setLocation( hostName, location );
   dc.setActiveLocation( location );

   // 不填参数
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      dc.setActiveLocation();
   } );

   dc.setLocation( hostName, "" );
}