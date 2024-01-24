/******************************************************************************
 * @Description   : seqDB-25383:回收站恢复一致性索引 
 * @Author        : liuli
 * @CreateTime    : 2022.02.24
 * @LastEditTime  : 2022.02.24
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipOneGroup = true;

main( test );
function test ()
{
   var csName = "cs_25383";
   var clName = "cl_25383";
   var indexName = "indexName_25383";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   dbcl.insert( docs );

   // 创建一致性索引
   dbcl.createIndex( indexName, { a: 1 } );
   checkIndexTask( "Create index", csName, clName, indexName, 0 );

   // 删除CL
   dbcs.dropCL( clName );

   // 恢复CL项目
   var recycleName = getOneRecycleName( db, csName + "." + clName );
   db.getRecycleBin().returnItem( recycleName );

   // 恢复后校验数据和索引一致性
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   commCheckIndexConsistent( db, csName, clName, indexName, true );

   // 校验索引任务不存在
   checkNoTask( csName, clName, "Create index" );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}