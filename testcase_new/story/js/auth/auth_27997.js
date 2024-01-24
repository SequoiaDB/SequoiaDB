/******************************************************************************
 * @Description   :  seqDB-27997 :: 创建管理员用户执行非监控类操作 
 * @Author        : Wu Yan
 * @CreateTime    : 2022.09.24
 * @LastEditTime  : 2023.08.07
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

// SEQUOIADBMAINSTREAM-9798
// main( test );
function test ()
{
   try
   {
      var userName = "user27997";
      var passwd = "passwd27997";
      var role = "admin";
      db.createUsr( userName, passwd, { Role: role } );
      var authDB = new Sdb( COORDHOSTNAME, COORDSVCNAME, userName, passwd );

      //ddl/dml/dql
      var csName = "cs27997";
      var clName = "cl27997";
      ddlAndDmlAndDqlOpr( authDB, csName, clName );

      //dcl
      var user1 = "testuser27997";
      authDB.createUsr( user1, passwd, { Role: role } );
      authDB.dropUsr( user1, passwd );

      //获取用户角色信息
      checkListUsers( authDB, userName, { "Role": "admin" } );

      //集群管理操作
      createAndRemoveNode( authDB );

      //指定鉴权用户连接catalog节点
      var cataDB = getCatalogConn( authDB, userName, passwd );
      println( "---cataDB=" + cataDB );
      ddlAndDmlAndDqlOpr( cataDB, csName, clName );
      cataDB.close();

      //选择一个数据节点执行数据操作      
      var dataDB = getDataConn( authDB, userName, passwd );
      println( "---dataDB=" + dataDB );
      ddlAndDmlAndDqlOpr( dataDB, csName, clName );
      dataDB.close();
   }
   finally
   {
      authDB.dropUsr( userName, passwd );
      authDB.close();
   }
}







