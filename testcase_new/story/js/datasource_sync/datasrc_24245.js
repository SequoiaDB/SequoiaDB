/******************************************************************************
 * @Description   : seqDB-24245:源集群设置不继承会话访问属性，单值指定preferedinstance存在的同时设置PreferedStrict
 * @Author        : Wu Yan
 * @CreateTime    : 2021.06.07
 * @LastEditTime  : 2022.11.03
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc24245";
   var clName = CHANGEDPREFIX + "_datasource24245";
   var srcCSName = "datasrcCS_24245";
   var csNameA = "DS_24245A";
   var csNameB = "DS_24245B";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( [csNameA, csNameB], dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   var groups = commGetGroups( datasrcDB );
   var groupName = groups[0][0].GroupName;
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 }, ReplSize: -1, Group: groupName } );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { InheritSessionAttr: false } );
   //集合空间级映射
   var csA = db.createCS( csNameA, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbclA = csA.getCL( clName );
   //集合级映射
   var csB = db.createCS( csNameB );
   var dbclB = csB.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var docs = [{ a: 1, b: 1, c: "testc" }, { a: 2, b: [1, "test"], c: "testsgasdgasdg" }, { a: 3, b: 234.3, c: { a: 1 } }, { a: 4, b: 4, c: "testtess4" }];
   dbclB.insert( docs );

   try
   {
      //数据源集群上配置节点instanceId
      var instanceid = [19, 218, 10];
      for( var i = 0; i < instanceid.length; i++ )
      {
         var hostName = groups[0][i + 1].HostName;
         var svcName = groups[0][i + 1].svcname;
         updateConf( datasrcDB, { instanceid: instanceid[i] }, { NodeName: hostName + ":" + svcName }, [SDB_RTN_CONF_NOT_TAKE_EFFECT, SDB_COORD_NOT_ALL_DONE] );
      }
      datasrcDB.getRG( groupName ).stop();
      datasrcDB.getRG( groupName ).start();
      commCheckBusinessStatus( datasrcDB );
      datasrcDB.invalidateCache();

      //设置PreferedStrict: true，指定instanceId节点异常，默认走主节点
      var optionsA = { PreferedInstance: instanceid[1], PreferedStrict: true };
      db.setSessionAttr( optionsA );
      datasrcDB.getRG( groupName ).getNode( groups[0][2].HostName, groups[0][2].svcname ).stop();
      //有可能停的节点是主节点，则需要重新选主      
      checkMasterNodeExist( datasrcDB, groupName );
      var masterNode = datasrcDB.getRG( groupName ).getMaster();
      var expAccessNodes = [masterNode.toString()];
      findAndCheckAccessNodes( dbclA, expAccessNodes );
      findAndCheckAccessNodes( dbclB, expAccessNodes );

      //设置PreferedStrict: false，指定instanceId节点不存在,默认走主节点
      var optionsB = { PreferedInstance: instanceid[1], PreferedStrict: false };
      checkAccessNodes( dbclA, expAccessNodes, optionsB );
      checkAccessNodes( dbclB, expAccessNodes, optionsB );

      //设置PreferedStrict: false，指定instanceId节点存在时,默认走主节点
      datasrcDB.getRG( groupName ).getNode( groups[0][2].HostName, groups[0][2].svcname ).start();
      commCheckBusinessStatus( datasrcDB );
      checkAccessNodes( dbclA, expAccessNodes, optionsA );
      checkAccessNodes( dbclB, expAccessNodes, optionsA );
   }
   finally
   {
      datasrcDB.getRG( groupName ).start();
      deleteConf( datasrcDB, { instanceid: 1 }, { GroupName: groupName }, [SDB_RTN_CONF_NOT_TAKE_EFFECT, SDB_COORD_NOT_ALL_DONE] );
      datasrcDB.getRG( groupName ).stop();
      datasrcDB.getRG( groupName ).start();
      commCheckBusinessStatus( datasrcDB );
   }

   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
   clearDataSource( [csNameA, csNameB], dataSrcName );
}

