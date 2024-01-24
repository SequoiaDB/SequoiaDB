/******************************************************************************
 * @Description   : seqDB-25633:dropCL后dropCS，重命名恢复CS后重命名恢复CL
 * @Author        : liuli
 * @CreateTime    : 2022.03.25
 * @LastEditTime  : 2022.03.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25633";
   var csNameNew = "cs_new_25633";
   var clName = "cl_25633";

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

   // 重命名恢复CL项目
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );

   // 指定重命名CS为原始CS名称
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      db.getRecycleBin().returnItemToName( recycleName, csName + "." + clName );
   } );

   // 指定重命名恢复为新的CS名称
   db.getRecycleBin().returnItemToName( recycleName, csNameNew + "." + clName );

   // 校验恢复后CL中数据正确
   var dbcl = db.getCS( csNameNew ).getCL( clName );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   commDropCS( db, csNameNew );
   cleanRecycleBin( db, csNameNew );
}