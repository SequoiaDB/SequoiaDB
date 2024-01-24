/******************************************************************************
 * @Description   : seqDB-31845:SdbDomain.setActiveLocation(<location>)参数校验
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.26
 * @LastEditTime  : 2023.06.08
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var domainName = "domain31845";
   commDropDomain( db, domainName );
   var groups = commGetDataGroupNames( db );
   var domain = commCreateDomain( db, domainName, groups );

   // 类型为非string
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      domain.setActiveLocation( 123 );
   } );

   var text = "";
   for( var i = 0; i < 257; i++ )
   {
      text += "a";
   }
   // 类型长度超出256
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      domain.setActiveLocation( text );
   } );

   var location = "";
   for( var i = 0; i < 255; i++ )
   {
      location += "b";
   }
   // 所有复制组的主节点设置location
   groupSetLocation( groups, location );
   // 类型长度为255
   domain.setActiveLocation( location );
   checkGroupActiveLocation( db, groups, location );
   // 不填参数
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      domain.setActiveLocation();
   } );

   groupSetLocation( groups, "" );
   commDropDomain( db, domainName );

}

function groupSetLocation ( groups, location )
{
   for( var i = 0; i < groups.length; i++ )
   {
      var rg = db.getRG( groups[i] );
      var node = rg.getMaster();
      node.setLocation( location );
   }
}