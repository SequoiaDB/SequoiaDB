/******************************************************************************
 * @Description   : seqDB-25392:dropItem()接口参数校验
 * @Author        : liuli
 * @CreateTime    : 2022.03.02
 * @LastEditTime  : 2022.06.28
 * @LastEditors   : liuli
 ******************************************************************************/
// 合法参数其他用例可以覆盖
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_25392";
   var clName = "cl_25392";

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
      recycle.dropItem( "" );
   } );

   // 指定不存在的回收站项目，不符合解析
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      recycle.dropItem( "norecycle_25392" );
   } );

   // 不指定必填参数
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      recycle.dropItem();
   } );

   // option 指定非法字段和非法值
   var options = [{ Async: "true" }, { Async: 1 }];
   for( var i in options )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         recycle.dropItem( recycleName, true, options[i] );
      } );
   }

   // recursive 指定非法值
   var recursives = ["true", 1];
   for( var recursive in recursives )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         recycle.dropItem( recycleName, recursive );
      } );
   }

   // 删除回收站项目后指定不存在的回收站项目，符合解析
   recycle.dropItem( recycleName );
   assert.tryThrow( SDB_RECYCLE_ITEM_NOTEXIST, function()
   {
      recycle.dropItem( recycleName );
   } );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}