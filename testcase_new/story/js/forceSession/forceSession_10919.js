/**************************************
 * @Description: seqDB-10919: 连接coord节点指定sessionID和options参数终止会话 
 * @author: Zhao xiaoni 
 * @Date: 2019-12-20
 **************************************/
testConf.skipStandAlone = true;

main( test );

function test () 
{
   var group = commGetGroups( db )[0];
   var hostName = group[1].HostName;
   var svcName = group[1].svcname;
   var nodeID = group[1].NodeID;

   //forceSession with nodeID
   var option = { NodeID: nodeID };
   forceSession( db, hostName, svcName, option );


   //forceSession with hostName svcName
   options = { HostName: hostName, svcname: svcName };
   forceSession( db, hostName, svcName, options );

   //forceSession with nodeID hostName svcName
   option = { NodeID: nodeID, HostName: hostName, svname: svcName };
   forceSession( db, hostName, svcName, options );
}

function forceSession ( db, hostName, svcName, options )
{
   var dataDB = new Sdb( hostName, svcName );
   var sessionID = dataDB.list( SDB_LIST_SESSIONS_CURRENT, { Global: false } ).next().toObj().SessionID;
   db.forceSession( sessionID, options );

   assert.tryThrow( [SDB_NETWORK_CLOSE, SDB_NETWORK], function()
   {
      dataDB.list( SDB_LIST_SESSIONS_CURRENT );
   } );
}

