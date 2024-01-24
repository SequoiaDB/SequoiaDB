/******************************************************************************
 * @Description   : seqDB-24149:源集群设置会话访问属性，单值指定preferedinstance存在的实例 
 * @Author        : Wu Yan
 * @CreateTime    : 2021.05.06
 * @LastEditTime  : 2022.11.03
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc24149";
   var clName = "_datasource24149";
   var srcCSName = "datasrcCS_24149";
   var csNameA = "DS_24149A";
   var csNameB = "DS_24149B";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( [csNameA, csNameB], dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 }, ReplSize: -1 } );
   var groups = commGetCLGroups( datasrcDB, srcCSName + "." + clName );
   var groupName = groups[0];
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   //集合空间级映射
   var csA = db.createCS( csNameA, { DataSource: dataSrcName, Mapping: srcCSName } );
   //集合级映射
   var csB = db.createCS( csNameB );
   var dbclB = csB.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var docs = [{ a: 1, b: 1, c: "testc" }, { a: 2, b: [1, "test"], c: "testsgasdgasdg" }, { a: 3, b: 234.3, c: { a: 1 } }, { a: 4, b: 4, c: "testtess4" }];
   dbclB.insert( docs );

   var instanceid = 4;
   var node = datasrcDB.getRG( groupName ).getSlave();
   var dbclA = csA.getCL( clName );

   try
   {
      datasrcDB.getRG( groupName ).start();
      updateConf( datasrcDB, { instanceid: instanceid }, { NodeName: node.toString() }, [SDB_RTN_CONF_NOT_TAKE_EFFECT, SDB_COORD_NOT_ALL_DONE] );
      node.stop();
      node.start();
      commCheckBusinessStatus( datasrcDB );
      datasrcDB.invalidateCache();

      var options = { PreferedInstance: instanceid };
      var expAccessNodes = [node.toString()];
      setSessionAndcheckAccessNodes( dbclA, expAccessNodes, options );
      setSessionAndcheckAccessNodes( dbclB, expAccessNodes, options );

      options = { PreferedInstance: [instanceid] };
      setSessionAndcheckAccessNodes( dbclA, expAccessNodes, options );
      setSessionAndcheckAccessNodes( dbclB, expAccessNodes, options );
   }
   finally
   {
      node.start();
      deleteConf( datasrcDB, { instanceid: 1 }, { NodeName: node.toString() }, [SDB_RTN_CONF_NOT_TAKE_EFFECT, SDB_COORD_NOT_ALL_DONE] );
      node.stop();
      node.start();
      commCheckBusinessStatus( datasrcDB );
   }

   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
   clearDataSource( [csNameA, csNameB], dataSrcName );
}

