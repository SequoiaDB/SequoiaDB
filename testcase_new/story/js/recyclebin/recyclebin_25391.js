/******************************************************************************
 * @Description   : seqDB-25391:returnItem()接口参数校验
 * @Author        : liuli
 * @CreateTime    : 2022.03.02
 * @LastEditTime  : 2022.04.13
 * @LastEditors   : liuli
 ******************************************************************************/
// 合法参数其他用例可以覆盖
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_25391";
   var clName = "cl_25391";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var recycle = db.getRecycleBin();
   var dbcs = db.createCS( csName );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 } } );
   dbcl.insert( { a: 1 } );

   dbcs.dropCL( clName );

   var recycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );
   // 指定为空串
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      recycle.returnItem( "" );
   } );

   // 指定不存在的回收站项目，不符合解析
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      recycle.returnItem( "norecycle_25391" );
   } );

   // 不指定必填参数
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      recycle.returnItem();
   } );

   // option 指定不存在的字段
   var options = [{ Enforced: "true" }, { Enforced: 1 }];
   for( var i in options )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         recycle.returnItem( recycleName, options[i] );
      } );
   }

   // 删除回收站项目后指定不存在的回收站项目，符合解析
   recycle.dropItem( recycleName );
   assert.tryThrow( SDB_RECYCLE_ITEM_NOTEXIST, function()
   {
      recycle.returnItem( recycleName );
   } );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}