/******************************************************************************
 * @Description   : seqDB-24244:源集群设置不继承会话访问属性，指定preferedinstance存在的实例
 *                  seqDB-22855:场景b：修改为继承会话属性
 * @Author        : Wu Yan
 * @CreateTime    : 2021.05.06
 * @LastEditTime  : 2022.11.03
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc24244";
   var clName = CHANGEDPREFIX + "_datasource24244";
   var srcCSName = "datasrcCS_24244";
   var csNameA = "DS_24244a";
   var csNameB = "DS_24244b";
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

   var instanceid = 4;
   var node = datasrcDB.getRG( groupName ).getSlave();
   var hostName = node.getHostName();
   var svcName = node.getServiceName();
   var masterNode = datasrcDB.getRG( groupName ).getMaster();

   try
   {
      datasrcDB.getRG( groupName ).start();
      updateConf( datasrcDB, { instanceid: instanceid }, { NodeName: hostName + ":" + svcName }, [SDB_RTN_CONF_NOT_TAKE_EFFECT, SDB_COORD_NOT_ALL_DONE] );
      node.stop();
      node.start();
      commCheckBusinessStatus( datasrcDB );
      datasrcDB.invalidateCache();

      var options = { PreferedInstance: instanceid };
      var expAccessNodes = [masterNode.toString()];
      checkAccessNodes( dbclA, expAccessNodes, options );
      checkAccessNodes( dbclB, expAccessNodes, options );


      options = { PreferedInstance: [instanceid, 'S'] };
      checkAccessNodes( dbclA, expAccessNodes, options );
      checkAccessNodes( dbclB, expAccessNodes, options );

      //testcase22855:场景b：修改为继承会话属性
      var ds = db.getDataSource( dataSrcName );
      ds.alter( { InheritSessionAttr: true } );
      findAndCheckAccessNodes( dbclA, [node.toString()] );
      findAndCheckAccessNodes( dbclB, [node.toString()] );
   }
   finally
   {
      node.start();
      deleteConf( datasrcDB, { instanceid: 1 }, { NodeName: hostName + ":" + svcName }, [SDB_RTN_CONF_NOT_TAKE_EFFECT, SDB_COORD_NOT_ALL_DONE] );
      node.stop();
      node.start();
      commCheckBusinessStatus( datasrcDB );
   }

   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
   clearDataSource( [csNameA, csNameB], dataSrcName );
}

