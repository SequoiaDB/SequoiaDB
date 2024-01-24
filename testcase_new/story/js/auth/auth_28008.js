/******************************************************************************
 * @Description   : seqDB-28008:创建监控用户执行SdbRecycelBin类监控操作
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.10.26
 * @LastEditTime  : 2022.11.09
 * @LastEditors   : Xu Mingxing
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME + "_28008";
testConf.clName = COMMCLNAME + "_28008";

main( test );
function test ( agrs )
{
   var userName1 = "user_28008_1";
   var userName2 = "user_28008_2";
   var cl = agrs.testCL;
   var groupNames = commGetCLGroups( db, testConf.csName + "." + testConf.clName );
   var masterNode = db.getRG( groupNames[0] ).getMaster();
   cl.insert( { a: 1 } );
   agrs.testCS.dropCL( testConf.clName );
   db.createUsr( userName1, userName1, { Role: "admin" } );
   db.createUsr( userName2, userName2, { Role: "monitor" } );
   try
   {
      // 指定鉴权用户连接coord节点
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName2, userName2 );
      sdb.getRecycleBin().count();
      sdb.getRecycleBin().getDetail();
      sdb.getRecycleBin().list();
      sdb.getRecycleBin().snapshot();

      // 指定鉴权用户连接catalog节点
      var sdb = getCatalogConn( db, userName2, userName2 );
      // SdbCS类监控管理操作
      sdb.getCS( "SYSCAT" ).listCollections().toString();

      // 指定鉴权用户连接data节点
      var sdb = new Sdb( masterNode.getHostName(), masterNode.getServiceName(), userName2, userName2 );
      sdb.getCS( testConf.csName ).listCollections().toString();
   }
   finally
   {
      db.dropUsr( userName2, userName2 );
      db.dropUsr( userName1, userName1 );
      sdb.close();
   }
}

