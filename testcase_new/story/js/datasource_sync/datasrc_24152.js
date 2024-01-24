/******************************************************************************
 * @Description   : seqDB-24152:源集群设置会话访问属性，指定preferedinstance为多个组中的instanceid 
 * @Author        : Wu Yan
 * @CreateTime    : 2021.05.06
 * @LastEditTime  : 2022.11.03
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataGroupNames = commGetDataGroupNames( datasrcDB );
   if( dataGroupNames.length < 3 )
   {
      println( "---data source clusters are at least two groups!" )
      return;
   }

   var dataSrcName = "datasrc24152";
   var clName = CHANGEDPREFIX + "_datasource2452";
   var srcCSName = "datasrcCS_24152";
   var csNameA = "DS_24152A";
   var csNameB = "DS_24152B";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( [csNameA, csNameB], dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   var groups = commGetGroups( datasrcDB );
   var srcGroupName = dataGroupNames[0];
   var destGroupName = dataGroupNames[1];
   var dscl = commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 }, ReplSize: -1, Group: srcGroupName } );
   dscl.split( srcGroupName, destGroupName, 50 );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   //集合空间级映射
   var csA = db.createCS( csNameA, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbclA = csA.getCL( clName );
   //集合级映射
   var csB = db.createCS( csNameB );
   var dbclB = csB.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   insertBulkData( dbclA );

   var instanceid = 10;
   var srcNode = datasrcDB.getRG( srcGroupName ).getSlave();
   var destNode = datasrcDB.getRG( destGroupName ).getSlave();

   try
   {
      updateConf( datasrcDB, { instanceid: instanceid }, { NodeName: srcNode.toString() }, [SDB_RTN_CONF_NOT_TAKE_EFFECT, SDB_COORD_NOT_ALL_DONE] );
      srcNode.stop();
      srcNode.start();

      updateConf( datasrcDB, { instanceid: instanceid }, { NodeName: destNode.toString() }, [SDB_RTN_CONF_NOT_TAKE_EFFECT, SDB_COORD_NOT_ALL_DONE] );
      destNode.stop();
      destNode.start();
      commCheckBusinessStatus( datasrcDB );
      datasrcDB.invalidateCache();

      var options = { PreferedInstance: instanceid };
      var expAccessNodes = [srcNode.toString(), destNode.toString()];
      checkAccessNodes( dbclA, expAccessNodes, options );
      checkAccessNodes( dbclB, expAccessNodes, options );
   }
   finally
   {
      srcNode.start();
      destNode.start();
      deleteConf( datasrcDB, { instanceid: 1 }, { NodeName: srcNode.toString() }, [SDB_RTN_CONF_NOT_TAKE_EFFECT, SDB_COORD_NOT_ALL_DONE] );
      srcNode.stop();
      srcNode.start();
      deleteConf( datasrcDB, { instanceid: 1 }, { NodeName: destNode.toString() }, [SDB_RTN_CONF_NOT_TAKE_EFFECT, SDB_COORD_NOT_ALL_DONE] );
      destNode.stop();
      destNode.start();
      commCheckBusinessStatus( datasrcDB );
   }

   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
   clearDataSource( [csNameA, csNameB], dataSrcName );
}

