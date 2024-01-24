/******************************************************************************
 * @Description   : seqDB-24263:源集群设置会话访问属性，指定preferedinstance和preferedInstanceMode
 * @Author        : Wu Yan
 * @CreateTime    : 2021.05.06
 * @LastEditTime  : 2022.11.03
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc24263";
   var clName = CHANGEDPREFIX + "_datasource24263";
   var srcCSName = "datasrcCS_24263";
   var csNameA = "DS_24263A";
   var csNameB = "DS_24263B";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( [csNameA, csNameB], dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   var groups = commGetGroups( datasrcDB );
   var groupName = groups[0][0].GroupName;
   if( groups[0][0].Length < 3 )
   {
      println( "---At least three nodes in the group" );
      return;
   }
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 }, ReplSize: -1, Group: groupName } );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
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
      var instanceid = [7, 8, 10];
      var nodes = [];
      for( var i = 0; i < instanceid.length; i++ )
      {
         var hostName = groups[0][i + 1].HostName;
         var svcName = groups[0][i + 1].svcname;
         updateConf( datasrcDB, { instanceid: instanceid[i] }, { NodeName: hostName + ":" + svcName }, [SDB_RTN_CONF_NOT_TAKE_EFFECT, SDB_COORD_NOT_ALL_DONE] );
         nodes.push( hostName + ":" + svcName );
      }
      datasrcDB.getRG( groupName ).stop();
      datasrcDB.getRG( groupName ).start();
      commCheckBusinessStatus( datasrcDB );
      datasrcDB.invalidateCache();

      //设置PreferedInstanceMode: ordered,默认访问第一个instanceid对应节点
      var optionsA = { PreferedInstance: [instanceid[0], instanceid[1], instanceid[2]], PreferedInstanceMode: "ordered" };
      db.setSessionAttr( optionsA );
      var expAccessNodes = [nodes[0]];
      setSessionAndcheckAccessNodes( dbclA, expAccessNodes, optionsA );
      setSessionAndcheckAccessNodes( dbclB, expAccessNodes, optionsA );

      //停止第一个instanceid对应节点，按顺序访问第二个节点
      datasrcDB.getRG( groupName ).getNode( groups[0][1].HostName, groups[0][1].svcname ).stop();
      var expAccessNodes = [nodes[1]];
      findAndCheckAccessNodes( dbclA, expAccessNodes );
      findAndCheckAccessNodes( dbclB, expAccessNodes );

      datasrcDB.getRG( groupName ).getNode( groups[0][1].HostName, groups[0][1].svcname ).start();
      commCheckBusinessStatus( datasrcDB );

      //设置设置PreferedInstanceMode: random,随机访问节点      
      var optionsB = { PreferedInstance: [instanceid[0], instanceid[1]], PreferedInstanceMode: "random" };
      var expAccessNodes = [nodes[0], nodes[1]];
      setSessionAndcheckAccessNodes( dbclA, expAccessNodes, optionsB );
      setSessionAndcheckAccessNodes( dbclB, expAccessNodes, optionsB );

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

