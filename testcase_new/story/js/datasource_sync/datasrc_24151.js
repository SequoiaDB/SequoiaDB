/******************************************************************************
 * @Description   : seqDB-24151:源集群设置会话访问属性，preferedinstance为M/S/A/-M/-S/-A 
 * @Author        : Wu Yan
 * @CreateTime    : 2021.05.06
 * @LastEditTime  : 2021.06.07
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;

main( test );
function test ()
{
   var dataSrcName = "datasrc24151";
   var clName = "DS_datasource24151";
   var srcCSName = "datasrcCS_24151";
   var csNameA = "DS_24151A";
   var csNameB = "DS_24151B";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( [csNameA, csNameB], dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   var groups = commGetGroups( datasrcDB )[0];;
   var groupName = groups[0].GroupName;
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

   var primaryPos = groups[0].PrimaryPos;
   var primaryNode = datasrcDB.getRG( groupName ).getMaster().toString();
   //指定为M/-M/m/-m
   println( "-----begin to access to M---" );
   var instances = ["M", "-M", "m", "-m"];
   var expAccessNodesA = [primaryNode];
   for( var i = 0; i < instances.length; i++ )
   {
      var options = { PreferedInstance: instances[i] };      
      setSessionAndcheckAccessNodes ( dbclA, expAccessNodesA, options );
      setSessionAndcheckAccessNodes ( dbclB, expAccessNodesA, options );
   }

   //指定为S/-S/s/-s
   println( "-----begin to access to S---" );
   var expAccessNodesB = [];
   for( var i = 1; i < groups.length; i++ )
   {
      if( i !== primaryPos )
      {
         expAccessNodesB.push( groups[i]["HostName"] + ":" + groups[i]["svcname"] );
      }
   }

   var instances = ["S", "-S", "s", "-s"];
   for( var i = 0; i < instances.length; i++ )
   {
      var options = { PreferedInstance: instances[i], PreferedPeriod:0 };
      setSessionAndcheckAccessNodes ( dbclA, expAccessNodesB, options );
      setSessionAndcheckAccessNodes ( dbclB, expAccessNodesB, options );
   }

   //指定为A/-A/a/-a
   println( "-----begin to access to A---" );
   var instances = ["A", "-A", "a", "-a"];
   var expAccessNodesC = getGroupNodes( datasrcDB, groupName );
   for( var i = 0; i < instances.length; i++ )
   {
      var options = { PreferedInstance: instances[i] }; 
      setSessionAndcheckAccessNodes ( dbclA, expAccessNodesC, options );
      setSessionAndcheckAccessNodes ( dbclB, expAccessNodesC, options );
   }

   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
   clearDataSource( [csNameA, csNameB], dataSrcName );
}

