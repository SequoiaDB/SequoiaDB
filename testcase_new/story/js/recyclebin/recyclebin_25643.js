/******************************************************************************
 * @Description   : seqDB-25643:dropCL后renameCS，重命名恢复CL项目 
 * @Author        : liuli
 * @CreateTime    : 2022.03.26
 * @LastEditTime  : 2022.03.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25643";
   var csNameNew = "cs_new_25643";
   var clName = "cl_25643";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   commDropCS( db, csNameNew );
   cleanRecycleBin( db, csNameNew );

   var dbcs = commCreateCS( db, csName );
   var dbcl = commCreateCL( db, csName, clName );
   var docs = insertBulkData( dbcl, 1000 );

   // 删除CL
   dbcs.dropCL( clName );

   // renameCS 
   db.renameCS( csName, csNameNew );

   var recycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );

   // 重命名恢复CL项目，指定renameCS之前的CS名
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      db.getRecycleBin().returnItemToName( recycleName, csName + "." + clName );
   } );

   // 重命名恢复CL项目，指定renameCS后的CS名
   db.getRecycleBin().returnItemToName( recycleName, csNameNew + "." + clName );

   // 恢复后校验数据
   var dbcl = db.getCS( csNameNew ).getCL( clName );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}