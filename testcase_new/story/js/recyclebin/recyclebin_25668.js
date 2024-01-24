/******************************************************************************
 * @Description   : seqDB-25668:returnItemToName()接口参数校验
 * @Author        : liuli
 * @CreateTime    : 2022.03.28
 * @LastEditTime  : 2022.04.13
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25668";
   var clName = "cl_25668";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );
   var recycle = db.getRecycleBin();

   // 执行truncate，获取回收站项目后删除回收站项目
   dbcl.truncate();
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Truncate" );
   recycle.dropItem( recycleName );

   // 插入数据后执行truncate
   var docs = insertBulkData( dbcl, 1000 );
   dbcl.truncate();

   // 删除CL
   dbcs.dropCL( clName );

   // recycleName参数校验
   // 重命名恢复不存在的项目
   assert.tryThrow( SDB_RECYCLE_ITEM_NOTEXIST, function()
   {
      recycle.returnItemToName( recycleName, csName + "." + clName );
   } );

   // 指定为空串
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      recycle.returnItemToName( "", csName + "." + clName );
   } );

   // 指定名称不符合命名规则
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      recycle.returnItemToName( "noItem", csName + "." + clName );
   } );

   // 不指定
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      recycle.returnItemToName();
   } );

   // returnName参数校验
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );

   // 指定为空串
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      recycle.returnItemToName( recycleName, "" );
   } );

   // 不指定
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      recycle.returnItemToName( recycleName );
   } );

   // 重命名恢复truncate项目，CL名称不符合CL命名规则
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Truncate" );

   // $ 起始
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      recycle.returnItemToName( recycleName, csName + "." + "$cl_25568" );
   } );

   // SYS 起始
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      recycle.returnItemToName( recycleName, csName + "." + "SYScl_25568" );
   } );

   // 包含 .
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      recycle.returnItemToName( recycleName, csName + "." + "c.l_25568" );
   } );

   // 长度超过127字节
   var arr = new Array( 123 );
   var clNameNew = arr.join( 'a' );
   clNameNew = clNameNew + "cl25668";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      recycle.returnItemToName( recycleName, csName + "." + clNameNew );
   } );

   // 重命名恢复dropCL项目，CL名称不符合CL命名规则
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );

   // $ 起始
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      recycle.returnItemToName( recycleName, csName + "." + "$cl_25568" );
   } );

   // SYS 起始
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      recycle.returnItemToName( recycleName, csName + "." + "SYScl_25568" );
   } );

   // 包含 .
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      recycle.returnItemToName( recycleName, csName + "." + "c.l_25568" );
   } );

   // 长度超过127字节
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      recycle.returnItemToName( recycleName, csName + "." + clNameNew );
   } );

   // 删除CS
   db.dropCS( csName );

   // 重命名恢复dropCS项目，CS名称不符合CS命名规则
   var recycleName = getOneRecycleName( db, csName, "Drop" );

   // $ 起始
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      recycle.returnItemToName( recycleName, "$cs_25568" );
   } );

   // SYS 起始
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      recycle.returnItemToName( recycleName, "SYScs_25568" );
   } );

   // 包含 .
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      recycle.returnItemToName( recycleName, "c.s_25568" );
   } );

   // 长度超过127字节
   var arr = new Array( 123 );
   var csNameNew = arr.join( 'a' );
   csNameNew = csNameNew + "cs25668";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      recycle.returnItemToName( recycleName, csNameNew );
   } );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}