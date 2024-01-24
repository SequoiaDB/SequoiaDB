/******************************************************************************
 * @Description   : seqDB-31835:通过域修改ActiveLocation
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.29
 * @LastEditTime  : 2023.06.09
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location1 = "location_31835_1";
   var location2 = "location_31835_2";
   var location3 = "location_31835_3";
   var domainName = "domain31835";
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
   // 所有复制组的备节点设置location
   for( var i = 0; i < groups.length; i++ )
   {
      var rg = db.getRG( groups[i] );
      var node = rg.getSlave();
      node.setLocation( location2 );
   }

   // 设置所有复制组中存在的location
   domain.setActiveLocation( location1 );
   checkGroupActiveLocation( db, groups, location1 );

   // 重复修改Activelocation
   domain.setActiveLocation( location1 );
   checkGroupActiveLocation( db, groups, location1 );

   // 修改ActiveLocation为复制组都存在的location
   domain.setActiveLocation( location2 );
   checkGroupActiveLocation( db, groups, location2 );

   // 设置所有复制组中不存在的location
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      domain.setActiveLocation( location3 );
   } );

   domain.removeGroups( { Groups: groups[0] } );
   var groups = commGetDataGroupNames( db );
   domain.setActiveLocation( location1 );
   checkGroupActiveLocation( db, groups, location1 );

   domain.addGroups( { Groups: groups[0] } );
   var groups = commGetDataGroupNames( db );
   domain.setActiveLocation( location2 );
   checkGroupActiveLocation( db, groups, location2 );
   commDropDomain( db, domainName );
}