/******************************************************************************
 * @Description   : seqDB-25640:truncate后dropCL，重命名恢复dropCL项目，直接恢复truncate项目 
 * @Author        : liuli
 * @CreateTime    : 2022.03.26
 * @LastEditTime  : 2022.03.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25640";
   var clName = "cl_25640";
   var clNameNew = "cl_new_25640";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = commCreateCL( db, csName, clName );
   var docs = insertBulkData( dbcl, 1000 );

   // CL执行truncate
   dbcl.truncate();

   // 删除CL
   dbcs.dropCL( clName );

   // 重命名恢复CL项目
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );
   db.getRecycleBin().returnItemToName( recycleName, csName + "." + clNameNew );

   // 恢复后校验数据
   var dbcl = dbcs.getCL( clNameNew );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, [] );

   // 恢复truncate项目
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Truncate" );
   assert.tryThrow( SDB_RECYCLE_CONFLICT, function()
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 强制恢复truncate项目
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );

   // 校验CL中数据正确
   var dbcl = dbcs.getCL( clName );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   // 校验clNameNew不存在
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      dbcs.getCL( clNameNew );
   } );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}