/******************************************************************************
 * @Description   : seqDB-26413:主表和固定集合不支持无事务属性
 * @Author        : liuli
 * @CreateTime    : 2022.04.24
 * @LastEditTime  : 2022.04.24
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( args )
{
   var csName = "cs_26413";
   var mainclName = "maincl_26413";
   var clName = "cl_26413";

   var dbcs = testPara.testCS;
   commDropCS( db, csName );
   var cappedCS = db.createCS( csName, { Capped: true } );

   // 创建主表指定NoTrans为true
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      dbcs.createCL( mainclName, { IsMainCL: true, ShardingKey: { a: 1 }, NoTrans: true } );
   } );

   // 创建固定集合指定NoTrans为true
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      cappedCS.createCL( clName, { Capped: true, Size: 1000, NoTrans: true } );
   } );

   // 创建主表指定NoTrans为false
   var maincl = dbcs.createCL( mainclName, { IsMainCL: true, ShardingKey: { a: 1 }, NoTrans: false } );

   // 创建固定集合指定NoTrans为false
   var dbcl = cappedCS.createCL( clName, { Capped: true, Size: 1000, NoTrans: false } );

   // 修改主表NoTrans为true
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      maincl.alter( { NoTrans: true } );
   } );
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      maincl.setAttributes( { NoTrans: true } );
   } );

   // 修改固定集合NoTrans为true
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      dbcl.alter( { NoTrans: true } );
   } );
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      dbcl.setAttributes( { NoTrans: true } );
   } );

   // 修改主表NoTrans为false
   maincl.alter( { NoTrans: false } );
   maincl.setAttributes( { NoTrans: false } );

   // 修改固定集合NoTrans为false
   dbcl.alter( { NoTrans: false } );
   dbcl.setAttributes( { NoTrans: false } );

   commDropCS( db, csName );
}