/******************************************************************************
 * @Description   : seqDB-25634:重命名恢复CL项目 
 * @Author        : liuli
 * @CreateTime    : 2022.03.25
 * @LastEditTime  : 2022.03.28
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25634";
   var clName = "cl_25634";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = commCreateCL( db, csName, clName );
   var docs = insertBulkData( dbcl, 1000 );

   // 删除CL
   dbcs.dropCL( clName );

   // 重命名恢复CL项目
   var arr = new Array( 122 );
   var clNameNew = arr.join( 'a' );
   clNameNew = clNameNew + "25634";
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );
   db.getRecycleBin().returnItemToName( recycleName, csName + "." + clNameNew );

   // 使用原始CL执行数据操作
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      dbcl.insert( { a: 1 } );
   } );

   // 校验恢复后CL中数据正确
   var dbcl = db.getCS( csName ).getCL( clNameNew );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}