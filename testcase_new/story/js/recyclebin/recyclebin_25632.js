/******************************************************************************
 * @Description   : seqDB-25632:dropCL后dropCS，重命名恢复CS后恢复CL 
 * @Author        : liuli
 * @CreateTime    : 2022.03.25
 * @LastEditTime  : 2022.03.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_25632";
   var clName = "cl_25632";
   var csNameNew = "cs_new_25632";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   commDropCS( db, csNameNew );
   cleanRecycleBin( db, csNameNew );

   var dbcs = commCreateCS( db, csName );
   var dbcl = commCreateCL( db, csName, clName );
   var docs = insertBulkData( dbcl, 1000 );

   // 删除CL后删除CS
   dbcs.dropCL( clName );
   db.dropCS( csName );

   // 重命名恢复CS项目
   var recycleName = getOneRecycleName( db, csName, "Drop" );
   db.getRecycleBin().returnItemToName( recycleName, csNameNew );

   // 直接恢复CL项目
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );
   assert.tryThrow( SDB_RECYCLE_CONFLICT, function()
   {
      db.getRecycleBin().returnItem( recycleName );
   } );
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );

   // 使用原始获取的C进行数据操作
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      dbcl.insert( { a: 1 } );
   } );

   // 校验恢复后CL中数据正确
   var dbcl = db.getCS( csNameNew ).getCL( clName );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   commDropCS( db, csNameNew );
   cleanRecycleBin( db, csNameNew );
}