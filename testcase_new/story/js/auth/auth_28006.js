/******************************************************************************
 * @Description   : seqDB-28006:创建监控用户执行SdbNode类监控操作
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.09.29
 * @LastEditTime  : 2022.10.09
 * @LastEditors   : Xu Mingxing
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var userName1 = "user_28006_1";
   var userName2 = "user_28006_2";
   var csName = "csName_28006";
   var clName = "clName_28006";
   var groupName = commGetDataGroupNames( db )[0];
   var cl = db.createCS( csName ).createCL( clName );
   cl.insert( { a: 1 } );
   db.createUsr( userName1, userName1, { Role: "admin" } );
   db.createUsr( userName2, userName2, { Role: "monitor" } );
   try
   {
      // 指定鉴权用户连接coord节点
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName2, userName2 );
      var node = sdb.getRG( groupName ).getMaster();
      node.connect();
      node.getHostName();
      node.getDetailObj();
      node.getServiceName();

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
