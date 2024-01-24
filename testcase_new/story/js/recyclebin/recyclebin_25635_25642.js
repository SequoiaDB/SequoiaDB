/******************************************************************************
 * @Description   : seqDB-25635:重命名恢复CL项目，指定CL名已存在 
 *                : seqDB-25642:重命名恢复CL，指定名称为原始名称 
 * @Author        : liuli
 * @CreateTime    : 2022.03.25
 * @LastEditTime  : 2022.03.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25635";
   var clName = "cl_25635";
   var clNameNew = "cl_new_25635";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = commCreateCL( db, csName, clName );
   var dbcl2 = commCreateCL( db, csName, clNameNew );
   var docs = insertBulkData( dbcl, 1000 );
   var docs2 = insertBulkData( dbcl2, 500 );

   // 删除CL
   dbcs.dropCL( clName );

   // 重命名恢复CL项目，指定重命名恢复为一个已存在的CL
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );
   assert.tryThrow( SDB_DMS_EXIST, function()
   {
      db.getRecycleBin().returnItemToName( recycleName, csName + "." + clNameNew );
   } );

   // 重命名恢复CL项目，指定重命名为原始名称
   db.getRecycleBin().returnItemToName( recycleName, csName + "." + clName );

   // 校验恢复后CL中数据正确
   var dbcl = db.getCS( csName ).getCL( clName );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );
   var cursor = dbcl2.find().sort( { a: 1 } );
   commCompareResults( cursor, docs2 );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}