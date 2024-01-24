/**************************************
 * @Description: seqDB-10918 指定sessionID终止当前节点会话
 * @author:Zhao xiaoni 
 * @Date: 2019-12-20
 **************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var cata = commGetGroups( db, true, "SYSCatalogGroup", false );
   for( var i = 1; i < cata[0].length; i++ )
   {
      var cataHostName = cata[0][i].HostName;
      var cataSvcName = cata[0][i].svcname;
   }

   var data = commGetGroups( db );
   for( var i = 1; i < data[0].length; i++ )
   {
      var dataHostName = data[0][i].HostName;
      var dataSvcName = data[0][i].svcname;
   }

   forceSession( COORDHOSTNAME, COORDSVCNAME );
   forceSession( cataHostName, cataSvcName );
   forceSession( dataHostName, dataSvcName );
}

function forceSession ( hostName, svcName )
{
   var db = new Sdb( hostName, svcName );
   var oldSessionId = db.list( SDB_LIST_SESSIONS_CURRENT, { Global: false } ).next().toObj().SessionID;

   db.forceSession( oldSessionId );

   var sessionList = db.list( SDB_LIST_SESSIONS_CURRENT, { Global: false, SessionID: oldSessionId } ).toArray();
   assert.notEqual( sessionList.length, 0 );
}
