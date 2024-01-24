/**************************************
 * @Description: seqDB-10920: 非coord节点指定options参数终止会话  
 * @author: Zhao xiaoni 
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

   // 连catalog，配置正确options
   forceSession( cataHostName, cataSvcName, true );
   // 连catalog，配置不匹配options
   forceSession( cataHostName, cataSvcName, false );
   // 连data, 配置正确options
   forceSession( dataHostName, dataSvcName, true );
   // 连data, 配置不匹配options
   forceSession( dataHostName, dataSvcName, false );
}

function forceSession ( hostName, svcName, flag )
{
   var db1 = new Sdb( hostName, svcName );
   var db2 = new Sdb( hostName, svcName );

   var oldSessionID = db1.list( SDB_LIST_SESSIONS_CURRENT ).next().toObj().SessionID;
   var options = flag === true ? { "NodeName": hostName + ":" + svcName } : { "NodeID": 4321 }
   db2.forceSession( oldSessionID, options );

   assert.tryThrow( [SDB_NETWORK, SDB_NETWORK_CLOSE], function()
   {
      db1.list( SDB_LIST_SESSIONS );
   } );
}
