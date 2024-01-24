/****************************************************
*@Description: [seqDB-22502] Using curl command to obtain 'SDB_SNAP_INDEXSTATS' snapshot info
               通过curl语句 获取SDB_SNAP_INDEXSTATS快照信息
*@Author:   2020-08-28  Zixian Yan
****************************************************/
testConf.csName = COMMCSNAME + "_22502";
testConf.clName = COMMCLNAME + "_22502";
if( !commIsStandalone( db ) )
{
   var groupInfo = commGetGroups( db );
   var GROUPNAME = groupInfo[0][0]["GroupName"];
   testConf.clOpt = { Group: GROUPNAME };
}
var collection = testConf.csName + "." + testConf.clName;
main( test );

function test ( testPara )
{
   var cl = testPara.testCL;
   var indexName = "index_22502";
   var data = [{ a: 1 }, { a: 2 }];

   cl.insert( data );
   cl.createIndex( indexName, { a: 1 } );
   db.analyze( { Collection: collection, Index: indexName } );

   var port = parseInt( COORDSVCNAME ) + 4;
   var url = "http://" + COORDHOSTNAME + ":" + port;
   if( !commIsStandalone( db ) )
   {
      // Query master node information
      var masterNode = db.getRG( GROUPNAME ).getMaster();
      var filter = { "Collection": collection, "Index": indexName, "NodeName": "" + masterNode + "" };

   }
   else
   {
      var filter = { "Collection": collection, "Index": indexName };
   }

   var condition = "'cmd=snapshot index statistics&filter=" + JSON.stringify( filter ) + "'";
   var cmd = new Cmd();
   var curl = "curl -X POST -i " + url + " -d " + condition + " -H  'Accept: application/json' 2>/dev/null";
   var result = cmd.run( curl );
   result = result.split( "\n" )[7];
   result = JSON.parse( result );

   // Result Check
   var errorInfo = result[0];
   if( errorInfo.errno != 0 )
   {
      throw new Error( "Curl Command Failed!!!! msg = " + condition );
   }

   var detailInfo = result[1];
   if( !commIsStandalone( db ) )
   {
      var maxValue = detailInfo.StatInfo[0].Group[0].MaxValue.a;
      var minValue = detailInfo.StatInfo[0].Group[0].MinValue.a;
   }
   else
   {
      var maxValue = detailInfo.MaxValue.a;
      var minValue = detailInfo.MinValue.a;
   }
   if( maxValue != 2 || minValue != 1 )
   {
      throw new Error( "Failed!!!! Something go wrong! \nExpected Value      Actually Value\nMaxvalue: 2         MaxValue: " + maxValue + "\nMinvalue: 1         MinValue: " + minValue );
   }
}
