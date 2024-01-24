/******************************************************************************
 * @Description   : seqDB-23827:强制恢复回收站某个对象
 * @Author        : liuli
 * @CreateTime    : 2021.04.15
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_23827";
   var clName = "cl_23827";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "tests2", c: "test" }, { a: 3, b: { b: 1 } }];

   dbcl.insert( docs );

   // 删除CL后创建同名CL
   dbcs.dropCL( clName );
   dbcs.createCL( clName );

   // 恢复truncate项目
   var recycleName = getOneRecycleName( db, csName + "." + clName );
   assert.tryThrow( [SDB_RECYCLE_CONFLICT], function() 
   {
      db.getRecycleBin().returnItem( recycleName, { Enforced: false } );
   } );

   // 恢复指定无效值
   assert.tryThrow( [SDB_INVALIDARG], function() 
   {
      db.getRecycleBin().returnItem( recycleName, { Enforced: 0 } );
   } );

   assert.tryThrow( [SDB_INVALIDARG], function() 
   {
      db.getRecycleBin().returnItem( recycleName, { Enforced: 1 } );
   } );

   assert.tryThrow( [SDB_INVALIDARG], function() 
   {
      db.getRecycleBin().returnItem( recycleName, { Enforced: null } );
   } );

   assert.tryThrow( [SDB_INVALIDARG], function() 
   {
      db.getRecycleBin().returnItem( recycleName, { Enforced: "" } );
   } );

   // 强制恢复
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}