/******************************************************************************
 * @Description   : seqDB-23763:指定索引名和子表，复制唯一索引   
 * @Author        : wu yan
 * @CreateTime    : 2021.04.07
 * @LastEditTime  : 2021.11.26
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var indexName = "Index_23763";
   var mainCLName = COMMCLNAME + "_maincl_index23763";
   var subCLName1 = COMMCLNAME + "_subcl_index23763a";
   var subCLName2 = COMMCLNAME + "_subcl_index23763b";
   var recordNum = 20000;

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "Fail to drop CL in the beginning" );
   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   maincl.createIndex( indexName, { a: 1, b: 1 }, true, true );
   commCreateCL( db, COMMCSNAME, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 10000 } } );
   maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 10000 }, UpBound: { a: 20000 } } );

   //主表上复制索引到指定子表
   var expRecs = insertBulkData( maincl, recordNum );
   maincl.copyIndex( COMMCSNAME + "." + subCLName2, indexName );

   //检查索引任务和索引信息   
   checkCopyTask( COMMCSNAME, mainCLName, [indexName], [COMMCSNAME + "." + subCLName2] );
   checkIndexTask( "Create index", COMMCSNAME, subCLName2, [indexName] );
   checkIndexExist( db, COMMCSNAME, subCLName1, indexName, false );
   checkIndexExist( db, COMMCSNAME, subCLName2, indexName, true );
   //查询匹配子表1和子表2范围数据
   checkExplainByMaincl( maincl, { a: 1, b: 1 }, "ixscan", "$shard" );
   checkExplainByMaincl( maincl, { a: 10000, b: 10000 }, "ixscan", indexName );

   //检查数据
   var cursor = maincl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } ).hint( { "": indexName } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );

   //插入重复数据到子表
   var expRec1 = { a: 1, b: 1 };
   var expCount1 = 2;
   maincl.insert( [expRec1, expRec1] );
   var count1 = maincl.count( expRec1 );
   assert.equal( expCount1, count1 );

   var expRec2 = { a: 10000, b: 20000 };
   var expCount2 = 1;
   maincl.insert( expRec2 );
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      maincl.insert( expRec2 );
   } );
   var count2 = maincl.count( expRec2 );
   assert.equal( count2, expCount2 );

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "Fail to drop CL in the ending" );
}
