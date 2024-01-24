/******************************************************************************
 * @Description   : seqDB-31834:通过域设置ActiveLocation
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.25
 * @LastEditTime  : 2023.06.08
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location1 = "location_31834_1";
   var location2 = "location_31834_2";
   var domainName = "domain31834";
   commDropDomain( db, domainName );
   var groups = commGetDataGroupNames( db );
   var domain = commCreateDomain( db, domainName, groups );

   // 所有复制组的主节点设置location
   for( var i = 0; i < groups.length; i++ )
   {
      var rg = db.getRG( groups[i] );
      var node = rg.getMaster();
      node.setLocation( location1 );
   }

   // 设置所有复制组中存在的location
   domain.setActiveLocation( location1 );
   checkGroupActiveLocation( db, groups, location1 );

   // 设置所有复制组中不存在的location
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      domain.setActiveLocation( location2 );
   } );

   // 修改其中一个复制组的location
   node.setLocation( location2 );

   // 设置部分复制组中存在的location
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      domain.setActiveLocation( location1 );
   } );
   commDropDomain( db, domainName );
}