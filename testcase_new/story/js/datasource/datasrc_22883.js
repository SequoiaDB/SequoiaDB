/******************************************************************************
 * @Description   : seqDB-22883:源集群上创建分区集合，关联数据源
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.16
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;

main( test );
function test ( testPara )
{
   var dataSrcName = "datasrc22883";
   var srcCSName = "datasrcCS_22883";
   var csName = "cs_22883";
   var clName = "cl_22883";
   var mainCLName = "mainCL_22883";
   var subCLName = "subCL_22883";
   var srcGroupName = testPara.srcGroupName;
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   var cs = db.createCS( csName );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      cs.createCL( clName, { ShardingKey: { a: 1 }, DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   } );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      cs.createCL( mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true, DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   } );

   clearDataSource( csName, dataSrcName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
}


