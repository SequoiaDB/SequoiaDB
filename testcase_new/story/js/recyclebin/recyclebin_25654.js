/******************************************************************************
 * @Description   : seqDB-25654:存在索引truncate后重命名恢复 
 * @Author        : liuli
 * @CreateTime    : 2022.03.28
 * @LastEditTime  : 2022.03.28
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25654";
   var clName = "cl_25654";
   var clNameNew = "cl_new_25654";
   var indexName = "index_25654";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );

   // 创建索引并插入数据
   dbcl.createIndex( indexName, { a: 1 } );
   var docs = insertBulkData( dbcl, 1000 );
   dbcl.truncate();

   // 重命名恢复truncate项目
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Truncate" );
   db.getRecycleBin().returnItemToName( recycleName, csName + "." + clNameNew );

   // 校验原始CL存在索引，不存在数据
   dbcl.getIndex( indexName );
   var cursor = dbcl.find();
   commCompareResults( cursor, [] );

   // 校验重命名恢复CL存在索引和数据
   var dbcl = dbcs.getCL( clNameNew );
   dbcl.getIndex( indexName );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}