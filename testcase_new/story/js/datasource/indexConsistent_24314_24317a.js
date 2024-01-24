/******************************************************************************
 * @Description   : seqDB-24314:使用数据源的集合为子表，主表上创建索引
 *                : seqDB-24317:使用数据源的集合为子表，主表上删除索引
 * @Author        : liuli
 * @CreateTime    : 2021.08.09
 * @LastEditTime  : 2021.09.24
 * @LastEditors   : liuli
 ******************************************************************************/
// 数据源属性ErrorControlLevel为low
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var dataSrcName = "datasrc24314_24317a";
   var csName = "cs_24314_24317a";
   var srcCSName = "datasrcCS_24314_24317a";
   var clName = "cl_24314_24317a";
   var mainCLName = "mainCL_24314_24317a";
   var subCLName = "subCL_24314_24317a";
   var indexName = "index_24314_24317a";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   // 主子表
   var srccl = commCreateCL( datasrcDB, srcCSName, clName );
   var dbcs = commCreateCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   maincl.attachCL( csName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( csName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );
   insertBulkData( maincl, 2000 );

   // 用例seqDB-24314的测试点，指定ErrorControlLevel为low
   // ErrorControlLevel为low，主表创建索引
   maincl.createIndex( indexName, { c: 1 } );

   // 校验任务
   checkIndexTask( "Create index", csName, mainCLName, indexName, 0 );
   checkIndexTask( "Create index", csName, subCLName, indexName, 0 );
   checkNoTask( csName, clName, "Create index" );

   // 校验索引
   checkIndexExist( db, csName, mainCLName, indexName, true );
   commCheckIndexConsistent( db, csName, subCLName, indexName, true );
   checkIndexExist( db, csName, clName, indexName, false );

   // 查看访问计划
   checkExplainByMaincl( maincl, csName + "." + mainCLName, { c: 5 }, ["ixscan", indexName], ["tbscan", ""] );
   checkExplainByMaincl( maincl, csName + "." + mainCLName, { c: 1005 }, ["ixscan", indexName], ["tbscan", ""] );

   // 主表listIndexes
   var cursor = maincl.listIndexes();
   while( cursor.next() )
   {
      var indexDef = cursor.current().toObj().IndexDef;
      assert.equal( indexDef.name, indexName );
      if( cursor.next() != undefined )
      {
         throw new Error( JSON.stringify( cursor.current().toObj() ) );
      }
   }
   cursor.close();

   // 用例seqDB-24317的测试点，指定ErrorControlLevel为low
   // 数据源集群端创建相同的索引
   srccl.createIndex( indexName, { c: 1 } );

   // 主表删除索引
   maincl.dropIndex( indexName );

   // 校验任务
   checkIndexTask( "Drop index", csName, mainCLName, indexName, 0 );
   checkIndexTask( "Drop index", csName, subCLName, indexName, 0 );
   checkNoTask( csName, clName, "Drop index" );

   // 校验索引
   checkIndexExist( db, csName, mainCLName, indexName, false );
   commCheckIndexConsistent( db, csName, subCLName, indexName, false );
   checkIndexExist( db, csName, clName, indexName, false );

   // 查看访问计划
   checkExplainByMaincl( maincl, csName + "." + mainCLName, { c: 5 }, ["tbscan", ""], ["ixscan", indexName] );
   checkExplainByMaincl( maincl, csName + "." + mainCLName, { c: 1005 }, ["tbscan", ""], ["ixscan", indexName] );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

// localCLExplain为本地集合的访问计划，srcCLExplain为数据源集合的访问计划
function checkExplainByMaincl ( maincl, mainCLName, cond, localCLExplain, srcCLExplain )
{
   var cursor = maincl.find( cond ).explain();
   while( cursor.next() )
   {
      var explain = cursor.current().toObj();
      if( explain["Name"] == mainCLName )
      {
         var listIndex = explain;
      }
      else
      {
         var dataIndex = explain;
      }
   }
   cursor.close();
   var scanType = listIndex["SubCollections"][0].ScanType;
   var indexName = listIndex["SubCollections"][0].IndexName;
   assert.equal( scanType, localCLExplain[0] );
   assert.equal( indexName, localCLExplain[1] );
   assert.equal( dataIndex.ScanType, srcCLExplain[0] );
   assert.equal( dataIndex.IndexName, srcCLExplain[1] );
}