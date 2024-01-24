/******************************************************************************
 * @Description   : seqDB-31838:通过dataCenter修改ActiveLocation
 * @Author        : tangtao
 * @CreateTime    : 2023.05.26
 * @LastEditTime  : 2023.05.26
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location1 = "guangzhou.nansha_31838";
   var location2 = "guangzhou.panyu_31838";
   var location3 = "guangzhou.baiyun_31838";

   var dataGroupNames = commGetDataGroupNames( db );

   // get dc and set location
   var dc = db.getDC();
   for ( var i = 0; i < dataGroupNames.length; i++ )
   {
      var rgName = dataGroupNames[i];
      var rg = db.getRG( rgName );
      var masterNode = rg.getMaster();
      var slaveNode = rg.getSlave();
      masterNode.setLocation( location1 );
      slaveNode.setLocation( location2 );
   }

   var rgName = CATALOG_GROUPNAME;
   var rg = db.getRG( rgName );
   var masterNode = rg.getMaster();
   var slaveNode = rg.getSlave();
   masterNode.setLocation( location1 );
   slaveNode.setLocation( location2 );

   try
   {
      // 1、dc 设置一个都存在的location为ActiveLocation
      dc.setActiveLocation( location1 );
      for ( var i = 0; i < dataGroupNames.length; i++ )
      {
         var rgName = dataGroupNames[i];
         checkGroupActiveLocation( db, rgName, location1 );
      }
      var rgName = CATALOG_GROUPNAME;
      checkGroupActiveLocation( db, rgName, location1 );

      // 2、dc 重复设置一个相同的location为ActiveLocation
      dc.setActiveLocation( location1 );
      for ( var i = 0; i < dataGroupNames.length; i++ )
      {
         var rgName = dataGroupNames[i];
         checkGroupActiveLocation( db, rgName, location1 );
      }
      var rgName = CATALOG_GROUPNAME;
      checkGroupActiveLocation( db, rgName, location1 );

      // 3、不同组有不同的ActiveLocation，设置为同一个ActiveLocation
      for ( var i = 0; i < dataGroupNames.length; i++ )
      {
         var rgName = dataGroupNames[i];
         var rg = db.getRG( rgName );
         rg.setActiveLocation( location1 );
      }
      var rg = db.getCataRG();
      rg.setActiveLocation( location2 );

      dc.setActiveLocation( location2 );

      for ( var i = 0; i < dataGroupNames.length; i++ )
      {
         var rgName = dataGroupNames[i];
         checkGroupActiveLocation( db, rgName, location2 );
      }
      var rgName = CATALOG_GROUPNAME;
      checkGroupActiveLocation( db, rgName, location2 );

      // 4、部分组没有ActiveLocation，设置为相同的ActiveLocation
      for ( var i = 0; i < dataGroupNames.length; i++ )
      {
         var rgName = dataGroupNames[i];
         var rg = db.getRG( rgName );
         rg.setActiveLocation( "" );
      }
      var rg = db.getCataRG();
      rg.setActiveLocation( location2 );

      dc.setActiveLocation( location2 );

      for ( var i = 0; i < dataGroupNames.length; i++ )
      {
         var rgName = dataGroupNames[i];
         checkGroupActiveLocation( db, rgName, location2 );
      }
      var rgName = CATALOG_GROUPNAME;
      checkGroupActiveLocation( db, rgName, location2 );

      // 5、dc 修改ActiveLocation为一个部分组不存在的location
      for ( var i = 0; i < dataGroupNames.length; i++ )
      {
         var rgName = dataGroupNames[i];
         var rg = db.getRG( rgName );
         var masterNode = rg.getMaster();
         masterNode.setLocation( location3 );
      }
      assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function () {
         dc.setActiveLocation( location3 );
      } );
   }
   finally
   {
      // reset location
      for ( var i = 0; i < dataGroupNames.length; i++ )
      {
         var rgName = dataGroupNames[i];
         var rg = db.getRG( rgName );
         var slaveNodes = getGroupSlaveNodeName( db, rgName );
         var masterNode = rg.getMaster();
         masterNode.setLocation( "" );
         for ( var j = 0; j < slaveNodes.length; j++ )
         {
            var slaveNode = rg.getNode( slaveNodes[j] );
            slaveNode.setLocation( "" );
         }
      }
      var rgName = CATALOG_GROUPNAME;
      var rg = db.getCataRG();
      var slaveNodes = getGroupSlaveNodeName( db, rgName );
      var masterNode = rg.getMaster();
      masterNode.setLocation( "" );
      for ( var j = 0; j < slaveNodes.length; j++ )
      {
         var slaveNode = rg.getNode( slaveNodes[j] );
         slaveNode.setLocation( "" );
      }
   }
}
