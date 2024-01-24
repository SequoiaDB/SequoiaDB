/******************************************************************************
 * @Description   : seqDB-28005:创建监控用户执行SdbReplicaGroup类监控操作
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.09.29
 * @LastEditTime  : 2022.10.09
 * @LastEditors   : Xu Mingxing
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var userName1 = "user_28005_1";
   var userName2 = "user_28005_2";
   var csName = "csName_28005";
   var clName = "clName_28005";
   var groups = commGetGroups( db );
   var groupName = groups[0][0].GroupName;
   var hostName = groups[0][1].HostName;
   var svcName = groups[0][1].svcname;
   var cl = db.createCS( csName ).createCL( clName );
   cl.insert( { a: 1 } );
   db.createUsr( userName1, userName1, { Role: "admin" } );
   db.createUsr( userName2, userName2, { Role: "monitor" } );
   try
   {
      // 指定鉴权用户连接coord节点
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName2, userName2 );
      var rg = sdb.getRG( groupName );
      rg.getDetailObj();
      rg.getMaster();
      rg.getNode( hostName, svcName );
      rg.getSlave();

      // 指定鉴权用户连接catalog节点
      var sdb = getCatalogConn( db, userName2, userName2 );
      // SdbCS类监控管理操作
      sdb.getCS( "SYSCAT" ).listCollections().toString();

      // 指定鉴权用户连接data节点
      var groupNames = commGetCLGroups( db, csName + "." + clName );
      var masterNode = db.getRG( groupNames[0] ).getMaster();
      var sdb = new Sdb( masterNode.getHostName(), masterNode.getServiceName(), userName2, userName2 );
      // SdbCS类监控管理操作
      sdb.getCS( csName ).listCollections().toString();
   }
   finally
   {
      db.dropUsr( userName2, userName2 );
      db.dropUsr( userName1, userName1 );
      db.dropCS( csName );
      sdb.close();
   }
}