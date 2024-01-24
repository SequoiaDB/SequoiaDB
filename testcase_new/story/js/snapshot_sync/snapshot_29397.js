/******************************************************************************
* @Description   : seqDB-29397:相同线程QueryID递增，不同线程QueryID不同
* @Author        : Cheng Jingjing
* @CreateTime    : 2022.12.21
* @LastEditTime  : 2022.12.21
* @LastEditors   : Cheng Jingjing
******************************************************************************/
testConf.clName = COMMCLNAME + "_csName_29397"
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;

main( test );
function test ( testPara )
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

      // SDB_SNAP_QUERY
      var ret = sdb.snapshot( SDB_SNAP_QUERIES );
      var queryID = ret.current().toObj()["QueryID"];
      ret = sdb.snapshot( SDB_SNAP_QUERIES, { QueryID: queryID } );
      if( !ret.next() )
      {
         throw new Error( "Invalid QueryID in query snapshot. It must be " + queryID );
      }
      // SDB_SNAP_CONTEXTS
      queryID = getNewQueryID( queryID );
      ret = sdb.snapshot( SDB_SNAP_CONTEXTS, { "$and": [{ "Contexts.QueryID": queryID }, { "Contexts.Type": "COORD" }] } );
      //print( JSON.parse(ret.toString()) );
      if( !ret.next() )
      {
         throw new Error( "Invalid QueryID in contexts snapshot. It must be " + queryID );
      }
   }
   finally
   {
      sdb.deleteConf( { mongroupmask: "all:basic", monslowquerythreshold: 0 } );
      sdb.close();
   }
}

function getNewQueryID ( curQueryID )
{
   var queryIDPrefixStr = curQueryID.substr( 0, 18 );
   var sequence = parseInt( curQueryID.substr( 18, 8 ), 16 );
   var newQueryID = queryIDPrefixStr + numToHexStr( ++sequence, 8 );
   return newQueryID;
}

function numToHexStr ( num, totalStrlen )
{
   var numHexStr = num.toString( 16 );
   if( totalStrlen < numHexStr.length )
   {
      throw new Error( "Invalid total str len" );
   }
   else if( totalStrlen == numHexStr.length )
   {
      return numHexStr;
   }
   else
   {
      var ret = "";
      for( var i = 0; i < totalStrlen - numHexStr.length; i++ )
      {
         ret += "0";
      }
      ret += numHexStr;
      return ret;
   }
}