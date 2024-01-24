/******************************************************************************
 * @Description   : seqDB-23766:子表在多个组上，主表复制索引   
 * @Author        : wu yan
 * @CreateTime    : 2021.04.02
 * @LastEditTime  : 2021.11.26
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "range" };
testConf.clName = COMMCLNAME + "_index23766a";

main( test );
function test ( testPara )
{
   var indexName = "Index_23766";

   var mainCLName = COMMCLNAME + "_maincl_index23766";
   var subCLName2 = COMMCLNAME + "_subcl_index23766b";
   var recordNum = 20000;
   var srcGroupName = testPara.srcGroupName;
   var dstGroupName = testPara.dstGroupNames[0];

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "Fail to drop CL in the beginning" );
   var maincl = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   maincl.createIndex( indexName, { a: 1, b: 1 }, { NotArray: true } );

   commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true } );
   maincl.attachCL( COMMCSNAME + "." + testConf.clName, { LowBound: { a: 0 }, UpBound: { a: 10000 } } );
   maincl.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: 10000 }, UpBound: { a: 20000 } } );
   testPara.testCL.split( srcGroupName, dstGroupName, { a: 5000 } );

   //主表上复制索引到子表
   var expRecs = insertBulkData( maincl, recordNum );
   maincl.copyIndex( "", indexName );

   //检查索引任务和索引信息  
   checkCopyTask( COMMCSNAME, mainCLName, [indexName], [COMMCSNAME + "." + testConf.clName, COMMCSNAME + "." + subCLName2] );
   checkIndexTask( "Create index", COMMCSNAME, testConf.clName, [indexName] );
   checkIndexTask( "Create index", COMMCSNAME, subCLName2, [indexName] );
   checkIndexExist( db, COMMCSNAME, testConf.clName, indexName, true );
   checkIndexExist( db, COMMCSNAME, subCLName2, indexName, true );

   var cursor = maincl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );

   checkExplainByMaincl( maincl, { a: 1, b: 1 }, "ixscan", indexName );
   checkExplainByMaincl( maincl, { a: 10000, b: 10000 }, "ixscan", indexName );

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "Fail to drop CL in the ending" );
}
