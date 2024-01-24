/******************************************************************************
 * @Description   : seqDB-23415:源集群上创建集合指定cl属性参数 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23415";
   var csName = "cs_23415";
   var srcCSName = "datasrcCS_23415";
   var clName = "cl_23415";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcs = commCreateCS( db, csName );

   assert.tryThrow( [SDB_OPERATION_INCOMPATIBLE], function()
   {
      dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName, ShardingKey: { a: 1 }, ShardingType: "hash" } );
   } );

   assert.tryThrow( [SDB_OPERATION_INCOMPATIBLE], function()
   {
      dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName, ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 4096 } );
   } );

   assert.tryThrow( [SDB_OPERATION_INCOMPATIBLE], function()
   {
      dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName, ReplSize: -1 } );
   } );

   assert.tryThrow( [SDB_OPERATION_INCOMPATIBLE], function()
   {
      dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName, Compressed: true } );
   } );

   assert.tryThrow( [SDB_OPERATION_INCOMPATIBLE], function()
   {
      dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName, CompressionType: "lzw" } );
   } );

   assert.tryThrow( [SDB_OPERATION_INCOMPATIBLE], function()
   {
      dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName, ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   } );

   assert.tryThrow( [SDB_OPERATION_INCOMPATIBLE], function()
   {
      dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName, ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true } );
   } );

   assert.tryThrow( [SDB_OPERATION_INCOMPATIBLE], function()
   {
      dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName, AutoIndexId: true } );
   } );

   assert.tryThrow( [SDB_OPERATION_INCOMPATIBLE], function()
   {
      dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName, StrictDataMode: true } );
   } );

   assert.tryThrow( [SDB_OPERATION_INCOMPATIBLE], function()
   {
      dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName, AutoIncrement: { a: 1 } } );
   } );

   assert.tryThrow( [SDB_OPERATION_INCOMPATIBLE], function()
   {
      dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName, EnsureShardingIndex: true } );
   } );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}