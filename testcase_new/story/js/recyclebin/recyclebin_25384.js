/******************************************************************************
 * @Description   : seqDB-25384:回收站恢复独立索引 
 * @Author        : liuli
 * @CreateTime    : 2022.02.24
 * @LastEditTime  : 2022.03.11
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipOneGroup = true;

main( test );
function test ()
{
   var csName = "cs_25384";
   var clName = "cl_25384";
   var indexName = "indexName_25384";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { ReplSize: -1 } );
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   dbcl.insert( docs );

   // 创建独立索引
   var nodeName = getCLOneNodeName( db, csName + "." + clName );
   dbcl.createIndex( indexName, { a: 1 }, { Standalone: true }, { NodeName: nodeName } );

   // 删除CL
   dbcs.dropCL( clName );

   // 恢复CL项目
   var recycleName = getOneRecycleName( db, csName + "." + clName );
   db.getRecycleBin().returnItem( recycleName );

   // 恢复后校验数据和独立索引
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   checkStandaloneIndexOnNode( db, csName, clName, indexName, nodeName, true );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}