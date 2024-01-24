/******************************************************************************
 * @Description   : seqDB-23762 :: 指定索引名和子表，复制普通索引   
 * @Author        : wu yan
 * @CreateTime    : 2021.04.02
 * @LastEditTime  : 2021.04.02
 * @LastEditors   : wuyan
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var indexName = "Index_23762";
   var mainCLName = COMMCLNAME + "_maincl_index23762";
   var subCLName1 = COMMCLNAME + "_subcl_index23762a";
   var subCLName2 = COMMCLNAME + "_subcl_index23762b";
   var recordNum = 20000;

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "Fail to drop CL in the beginning" );
   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   maincl.createIndex( indexName, { a: 1, b: 1 }, { NotArray: false } );
   commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 10000 } } );
   maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 10000 }, UpBound: { a: 20000 } } );

   //主表上复制索引到指定子表
   var expRecs = insertBulkData( maincl, recordNum );
   maincl.copyIndex( COMMCSNAME + "." + subCLName1, indexName );

   //检查任务信息    
   checkCopyTask( COMMCSNAME, mainCLName, [indexName], [COMMCSNAME + "." + subCLName1] );
   checkIndexTask( "Create index", COMMCSNAME, subCLName1, [indexName] );

   //检查索引信息
   checkIndexExist( db, COMMCSNAME, subCLName1, indexName, true );
   checkIndexExist( db, COMMCSNAME, subCLName2, indexName, false );
   checkExplainByMaincl( maincl, { a: 1, b: 1 }, "ixscan", indexName );
   checkExplainByMaincl( maincl, { a: 10000, b: 10000 }, "ixscan", "$shard" );

   var cursor = maincl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "Fail to drop CL in the ending" );
}

function insertBulkData ( dbcl, recordNum )
{
   var doc = [];
   for( var i = 0; i < recordNum; i++ )
   {
      var bValue = ["test" + i];
      var cValue = parseInt( Math.random() * recordNum );
      doc.push( { a: i, b: bValue, c: cValue } );
   }
   dbcl.insert( doc );
   return doc;
}
