/************************************************************************
*@Description: [seqDB-22791] Reelecting the target node while it just start.
               选举刚启动的目标节点为主节点
*@Author:  2020/10/10  Zixian Yan
************************************************************************/
testConf.clName = COMMCLNAME + "_22791";
testConf.clOpt = {};
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true;
testConf.useSrcGroup = true;
main( test );

function test ( testPara )
{
   var groupsInfo = testPara.groups;
   var groupName = testPara.srcGroupName;

   var nodesInfo = [];
   for( var i in groupsInfo )
   {
      if( groupsInfo[i][0]["GroupName"] != groupName )
      {
         continue;
      }

      for( var index = 1; index < groupsInfo[i].length; index++ )
      {
         var node = groupsInfo[i][index];
         var tmpInfo = new Object();
         tmpInfo["HostName"] = node["HostName"];
         tmpInfo["svcname"] = node["svcname"];
         tmpInfo["NodeID"] = node["NodeID"];
         nodesInfo.push( tmpInfo );
      }

   }
   // Random a node as a target node
   var pos = Math.ceil( rand() * 10 ) % nodesInfo.length;
   var destNode = nodesInfo[pos];
   var rg = db.getRG( groupName );

   rg.getNode( destNode["HostName"], destNode["svcname"] ).stop();

   var foundPrimary = false;
   var timeout = 60;
   while( !foundPrimary )
   {
      timeout -= 1;
      sleep( 1000 );
      if( timeout == 0 )
      {
         throw new Error( " This Cluster Environment has not found primary node! " );
      }

      for( var i in nodesInfo )
      {
         if( i == pos ) { continue; }
         var cursor = db.snapshot( 6, { NodeID: nodesInfo[i]["NodeID"], RawData: true }, { IsPrimary: 1 } );
         if( cursor.current().toObj()['IsPrimary'] == true )
         {
            foundPrimary = true;
         }
      }
   }

   rg.getNode( destNode["HostName"], destNode["svcname"] ).start();

   rg.reelect( { NodeID: destNode["NodeID"] } );
   cursor = db.snapshot( 6, { NodeID: destNode["NodeID"], RawData: true }, { IsPrimary: 1 } );
   if( cursor.current().toObj()['IsPrimary'] != true )
   {
      throw new Error( "Reelecting failed" );
   }

}
