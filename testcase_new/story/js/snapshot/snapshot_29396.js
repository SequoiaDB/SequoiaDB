/******************************************************************************
* @Description   : seqDB-29396:QueryID中identifyID由tid和nodeID转换组成
* @Author        : Cheng Jingjing
* @CreateTime    : 2022.12.21
* @LastEditTime  : 2022.12.21
* @LastEditors   : Cheng Jingjing
******************************************************************************/
testConf.clName = COMMCLNAME + "_csName_29396"
testConf.skipStandAlone = true;
main( test );

function test( testPara )
{
   var coord = commGetGroups( db, true, "SYSCoord", true, false, true )[0][1];
   var hostname = coord.HostName;
   var svcname = coord.svcname;
   var sdb = new Sdb( hostname, svcname );

   try
   {
      // updateconf
      sdb.updateConf( { mongroupmask: "all:basic", monslowquerythreshold: 0 } );

      //insert
      insertRecs( testPara.testCL );

      // get identifyID
      var source = "client_and_coord_session_29396";
      sdb.setSessionAttr( { Source: source } );
      var ret = sdb.list( SDB_LIST_SESSIONS, { $and: [{ Source: source }, { Type: "Agent" }] }, { TID: 1 } );
      var tid = ret.current().toObj()["TID"];
      var nodeID = coord.NodeID;

      var identifyIDHexStr = numToHexStr( tid, 8 ) + numToHexStr( nodeID, 4 );
      pattern = "0x" + identifyIDHexStr + ".*";

      // get queryID
      var ret = sdb.snapshot(
      SDB_SNAP_QUERIES,
      new SdbSnapshotOption().cond( { QueryID: { $regex: pattern } } ).sort( { QueryID: -1 } )
      );
      var curQueryID = ret.current().toObj()["QueryID"];
      if ( "" == curQueryID )
      {
         throw new Error( "Query snapshot must have QueryID field" );
      }
   }
   finally
   {
      sdb.deleteConf( { mongroupmask: "all:basic", monslowquerythreshold: 0 } );
      sdb.close();
   }
}

function numToHexStr( num, totalStrlen )
{
   var numHexStr = num.toString(16);
   if ( totalStrlen < numHexStr.length )
   {
      throw new Error( "Invalid total str len" );
   }
   else if ( totalStrlen == numHexStr.length )
   {
      return numHexStr;
   }
   else
   {
      var ret = "";
      for ( var i = 0; i < totalStrlen - numHexStr.length; i++ )
      {
         ret += "0";
      }
      ret += numHexStr;
      return ret;
   }
}