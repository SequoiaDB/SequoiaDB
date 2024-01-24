/******************************************************************************
 * @Description   : seqDB-25644:重命名恢复truncate项目 
 * @Author        : liuli
 * @CreateTime    : 2022.03.26
 * @LastEditTime  : 2022.03.28
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25644";
   var clName = "cl_25644";
   var clNameNew = "cl_new_25644";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 }, AutoSplit: true } );
   var docs = insertBulkData( dbcl, 1000 );

   // 执行truncate
   dbcl.truncate();

   // 重命名恢复truncate项目
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Truncate" );
   var returnName = db.getRecycleBin().returnItemToName( recycleName, csName + "." + clNameNew );

   // 校验返回名称
   assert.equal( returnName.toObj().ReturnName, csName + "." + clNameNew );

   // 恢复后校验原始Cl不存在数据
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, [] );

   // 校验恢复Cl数据正确
   var dbcl = dbcs.getCL( clNameNew );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}