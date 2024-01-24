/******************************************************************************
 * @Description   :  seqDB-24318 :: 使用数据源的集合为子表，主表上复制索引(数据源上设置"ErrorControlLevel": "high")   
 * @Author        : wu yan
 * @CreateTime    : 2021.08.11
 * @LastEditTime  : 2022.01.20
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ( testPara )
{
   var indexName = "Index_24318b";
   var dataSrcName = "datasrc24318b";
   var csName = "cs_24138b";
   var srcCSName = "srccs_24318b";
   var srcCLName = "srccl_24318b";
   var mainCLName = COMMCLNAME + "_maincl_index24318b";
   var subCLName1 = COMMCLNAME + "_subcl_index24318a_1";
   var subCLName2 = COMMCLNAME + "_subcl_index24318b_2";
   var recordNum = 20000;

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { ErrorControlLevel: "high" } );
   var cs = db.createCS( csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   maincl.createIndex( indexName, { a: 1, b: 1 } );
   cs.createCL( subCLName1, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   cs.createCL( subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 10000 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 10000 }, UpBound: { a: 20000 } } );

   var expRecs = insertBulkData( maincl, recordNum );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      maincl.copyIndex( "", indexName );
   } );

   checkNoTask( csName, mainCLName, "Copy index" );
   checkNoTask( csName, subCLName1, "Create index" );
   checkNoTask( csName, subCLName2, "Create index" );
   var cursor = maincl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );

   checkExplain( maincl, { a: 1, b: 1 }, "tbscan", "" );
   checkExplainByMaincl( maincl, { a: 10000, b: 10000 }, "ixscan", "$shard" );

   commCheckIndexConsistent( db, csName, subCLName2, indexName, false );

   db.dropCS( csName );
   db.dropDataSource( dataSrcName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
}
