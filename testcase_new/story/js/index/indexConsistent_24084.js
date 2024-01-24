/******************************************************************************
 * @Description   : 删除已复制索引，再次复制同名索引（索引定义不同）   
 * @Author        : wu yan
 * @CreateTime    : 2021.08.02
 * @LastEditTime  : 2022.02.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var indexName = "Index_24084";
   var mainCLName = COMMCLNAME + "_maincl_index24084";
   var subCLName1 = COMMCLNAME + "_subcl_index24084a";
   var subCLName2 = COMMCLNAME + "_subcl_index24084b";
   var recordNum = 10000;

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "Fail to drop CL in the beginning" );
   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   maincl.createIndex( indexName, { no: 1, b: -1 } );
   var subcl1 = commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 10000 } } );

   //主表上复制索引到指定子表
   var expRecs = insertBulkData( maincl, recordNum );
   maincl.copyIndex( COMMCSNAME + "." + subCLName1, indexName );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName1, indexName, true );

   //删除索引，构造只有主表上存在索引场景
   maincl.dropIndex( indexName );
   checkIndexExist( db, COMMCSNAME, mainCLName, indexName, false );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName1, indexName, false );

   maincl.createIndex( indexName, { a: 1, no: 1 } );
   checkIndexExist( db, COMMCSNAME, mainCLName, indexName, true );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName1, indexName, true );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName2, indexName, true );

   subcl1.dropIndex( indexName );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName1, indexName, false );

   maincl.copyIndex( COMMCSNAME + "." + subCLName1, indexName );

   var cursor = maincl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );
   commCheckIndexConsistent( db, COMMCSNAME, subCLName1, indexName, true );

   //分别指定子表1和子表2范围数据查询
   checkExplainByMaincl( maincl, { no: 1 }, "tbscan", "" );
   checkExplainByMaincl( maincl, { a: 1, no: 1 }, "ixscan", indexName );

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "Fail to drop CL in the ending" );
}
