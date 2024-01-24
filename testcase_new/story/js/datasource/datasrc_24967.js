/******************************************************************************
 * @Description   : seqDB-24967:修改cs名后，从另一协调节点上读取cs内容
 * @Author        : 钟子明
 * @CreateTime    : 2022.01.13
 * @LastEditTime  : 2022.01.18
 * @LastEditors   : 钟子明
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test () 
{
   var coordArr = getCoordUrl( db );
   //当前集群协调节点数量小于2跳过测试
   if( coordArr.length < 2 ) 
   {
      return;
   }
   var db1 = new Sdb( coordArr[0] );
   var db2 = new Sdb( coordArr[1] );
   var dataSrcName = "ds_24967";
   var csName = "cs_24967";
   var renamecsName = "cs_rename24967";
   var clName = "cl_24967";

   commDropCS( db, renamecsName );
   commDropCS( datasrcDB, csName );
   clearDataSource( csName, dataSrcName );

   commCreateCL( datasrcDB, csName, clName );
   db1.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   db1.createCS( csName, { DataSource: dataSrcName } );

   db1.getCS( csName ).getCL( clName ).count();
   db2.getCS( csName ).getCL( clName ).count();

   db1.renameCS( csName, renamecsName );

   //此处如果成功，说明db2连接的catalog cache没有更新
   assert.tryThrow( [SDB_DMS_CS_NOTEXIST], function()
   {
      db2.getCS( csName ).getCL( clName ).count();
   } );

   db2.close();
   commDropCS( datasrcDB, csName );
   clearDataSource( renamecsName, dataSrcName );
   db1.close()
   datasrcDB.close();
}