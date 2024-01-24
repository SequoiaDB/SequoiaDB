/******************************************************************************
 * @Description   : seqDB-25645:重命名恢复truncate项目，指定CL已存在 
 *                : seqDB-25653:truncate后重命名恢复，指定重命名为原始名称 
 * @Author        : liuli
 * @CreateTime    : 2022.03.28
 * @LastEditTime  : 2022.03.28
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25645";
   var clName = "cl_25645";
   var clNameNew = "cl_new_25645";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 }, AutoSplit: true } );
   var dbcl2 = dbcs.createCL( clNameNew );
   insertBulkData( dbcl, 1000 );
   var docs = insertBulkData( dbcl2, 1000 );

   // 执行truncate
   dbcl.truncate();

   // 重命名恢复truncate项目，指定为原始CL名称
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Truncate" );
   assert.tryThrow( SDB_DMS_EXIST, function()
   {
      db.getRecycleBin().returnItemToName( recycleName, csName + "." + clName );
   } );

   // 重命名恢复truncate项目，指定已存在的CL
   assert.tryThrow( SDB_DMS_EXIST, function()
   {
      db.getRecycleBin().returnItemToName( recycleName, csName + "." + clNameNew );
   } );

   // 校验数据
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, [] );
   var cursor = dbcl2.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}