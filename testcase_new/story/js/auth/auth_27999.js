/******************************************************************************
 * @Description   : seqDB-27999:创建监控用户执行Sdb类监控操作
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.09.28
 * @LastEditTime  : 2023.08.07
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
// SEQUOIADBMAINSTREAM-9798
// main( test );

function test ()
{
   var userName1 = "user_27999_1";
   var userName2 = "user_27999_2";
   var csName = "csName_27999";
   var clName = "clName_27999";
   var groupName = commGetDataGroupNames( db )[0];
   var domainName = "domain";
   var sequenceName = "sequence";
   var cl = commCreateCL( db, csName, clName );
   cl.insert( { a: 1 } );
   db.createUsr( userName1, userName1, { Role: "admin" } );
   db.createUsr( userName2, userName2, { Role: "monitor" } );
   try
   {
      // 指定鉴权用户连接coord节点
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName2, userName2 );
      // close关闭会话
      sdb.close();
      // 各对象列取操作
      var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName2, userName2 );
      sdb.listBackup().toArray();
      sdb.listCollections().toArray();
      sdb.listCollectionSpaces().toArray();
      sdb.listDomains().toArray();
      sdb.listProcedures().toArray();
      sdb.listReplicaGroups().toArray();
      sdb.listSequences().toArray();
      sdb.listTasks().toArray();
      // 各对象元数据操作
      sdb.getRG( groupName );
      sdb.getCataRG();
      sdb.getCoordRG();
      db.createSpareRG();
      sdb.getSpareRG();
      db.createDomain( domainName );
      sdb.getDomain( domainName );
      db.createSequence( sequenceName );
      sdb.getSequence( sequenceName );
      sdb.getCS( csName );
      sdb.getSessionAttr();
      sdb.traceStatus();
      // 重置快照
      sdb.resetSnapshot();
      // 列出当前集群中的所有用户信息
      var options = { Role: "monitor" };
      checkListUsers( sdb, userName2, options );

      // 指定鉴权用户连接catalog节点
      var sdb = getCatalogConn( db, userName2, userName2 );
      sdb.list( SDB_LIST_COLLECTIONS ).toString();

      // 指定鉴权用户连接data节点
      var sdb = getDataConn( db, userName2, userName2 );
      sdb.list( SDB_LIST_COLLECTIONS ).toString();
   }
   finally
   {
      db.dropUsr( userName2, userName2 );
      db.dropUsr( userName1, userName1 );
      db.removeSpareRG();
      db.dropDomain( domainName );
      db.dropSequence( sequenceName );
      db.dropCS( csName );
      sdb.close();
   }
}