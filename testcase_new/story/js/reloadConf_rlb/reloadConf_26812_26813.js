/******************************************************************************
 * @Description   : seqDB-26812:reloadConf加载配置包含别名和正式名
 *                : seqDB-26813:重启节点加载配置包含别名和正式名
 * @Author        : liuli
 * @CreateTime    : 2022.08.09
 * @LastEditTime  : 2022.08.09
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var groupName = commGetDataGroupNames( db )[0];
   var replicaGroup = db.getRG( groupName );
   var data = replicaGroup.getSlave();
   var dataHostName = data.getHostName();
   var dataServiceName = data.getServiceName();

   var oma = new Oma( dataHostName, CMSVCNAME );

   try
   {
      var option = { NodeName: dataHostName + ":" + dataServiceName }
      var defaultConf = {
         preferedinstance: "M", preferredinstance: "M", preferedinstancemode: "random", preferredinstancemode: "random",
         preferedstrict: "FALSE", preferredstrict: "FALSE", preferedperiod: 60, preferredperiod: 60
      }
      // 覆盖部分别名部分正式名，reloadConf生效
      var config = { preferedinstance: "A", preferredinstancemode: "ordered", preferedstrict: "TRUE", preferredperiod: 30 };
      oma.updateNodeConfigs( dataServiceName, config );
      db.reloadConf();
      var expConf = {
         preferedinstance: "A", preferredinstance: "A", preferedinstancemode: "ordered", preferredinstancemode: "ordered",
         preferedstrict: "TRUE", preferredstrict: "TRUE", preferedperiod: 30, preferredperiod: 30
      }
      checkSnapshot( db, option, expConf );
      db.deleteConf( config );
      checkSnapshot( db, option, defaultConf );

      var config = { preferredinstance: "A", preferedinstancemode: "ordered", preferredstrict: "TRUE", preferedperiod: 30 };
      oma.updateNodeConfigs( dataServiceName, config );
      db.reloadConf();
      checkSnapshot( db, option, expConf );
      db.deleteConf( config );
      checkSnapshot( db, option, defaultConf );

      // 别名和正式名均存在且不相同，reloadConf生效
      var config = {
         preferredinstance: "A", preferedinstance: "M", preferedinstancemode: "ordered", preferredinstancemode: "ordered",
         preferredstrict: "TRUE", preferedstrict: "FALSE", preferedperiod: "30", preferredperiod: "90"
      };
      oma.updateNodeConfigs( dataServiceName, config );
      db.reloadConf();
      var expConf = {
         preferedinstance: "A", preferredinstance: "A", preferedinstancemode: "ordered", preferredinstancemode: "ordered",
         preferedstrict: "TRUE", preferredstrict: "TRUE", preferedperiod: 90, preferredperiod: 90
      }
      checkSnapshot( db, option, expConf );
      db.deleteConf( config );
      checkSnapshot( db, option, defaultConf );

      // 覆盖部分别名部分正式名，重启生效
      var config = { preferedinstance: "A", preferredinstancemode: "ordered", preferedstrict: "TRUE", preferredperiod: 30 };
      oma.updateNodeConfigs( dataServiceName, config );
      replicaGroup.stop();
      replicaGroup.start();
      commCheckBusinessStatus( db );
      var expConf = {
         preferedinstance: "A", preferredinstance: "A", preferedinstancemode: "ordered", preferredinstancemode: "ordered",
         preferedstrict: "TRUE", preferredstrict: "TRUE", preferedperiod: 30, preferredperiod: 30
      }
      checkSnapshot( db, option, expConf );
      db.deleteConf( config );
      checkSnapshot( db, option, defaultConf );

      var config = { preferredinstance: "A", preferedinstancemode: "ordered", preferredstrict: "TRUE", preferedperiod: 30 };
      oma.updateNodeConfigs( dataServiceName, config );
      replicaGroup.stop();
      replicaGroup.start();
      commCheckBusinessStatus( db );
      checkSnapshot( db, option, expConf );
      db.deleteConf( config );
      checkSnapshot( db, option, defaultConf );

      // 别名和正式名均存在且不相同，重启生效
      var config = {
         preferredinstance: "A", preferedinstance: "M", preferedinstancemode: "ordered", preferredinstancemode: "ordered",
         preferredstrict: "TRUE", preferedstrict: "FALSE", preferedperiod: "30", preferredperiod: "90"
      };
      oma.updateNodeConfigs( dataServiceName, config );
      replicaGroup.stop();
      replicaGroup.start();
      commCheckBusinessStatus( db );
      var expConf = {
         preferedinstance: "A", preferredinstance: "A", preferedinstancemode: "ordered", preferredinstancemode: "ordered",
         preferedstrict: "TRUE", preferredstrict: "TRUE", preferedperiod: 90, preferredperiod: 90
      }
      checkSnapshot( db, option, expConf );
   } finally
   {
      db.deleteConf( { preferredinstance: 1, preferredinstancemode: 1, preferredstrict: 1, preferredperiod: 1 } );
      oma.close();
   }
}

function checkSnapshot ( sdb, option, expConf )
{
   var cursor = sdb.snapshot( SDB_SNAP_CONFIGS, option );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      for( var key in expConf )
      {
         assert.equal( obj[key], expConf[key] );
      }
   }
   cursor.close();
}