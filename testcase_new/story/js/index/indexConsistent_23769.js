/******************************************************************************
 * @Description   : seqDB-23769:重复创建相同索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.09.02
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_23769";
   var mainCLName = "maincl_23769";
   var subCLName1 = "subcl_23769_1";
   var subCLName2 = "subcl_23769_2";
   var indexName1 = "index_23769_1";
   var indexName2 = "index_23769_2";
   var indexName3 = "index_23769_3";

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 500 }, UpBound: { a: 1000 } } );

   // 主表创建索引
   maincl.createIndex( indexName1, { c: 1 } );

   // 主表中插入数据
   var docs = insertBulkData( maincl, 1000 );

   // 再次创建索引 a.索引名相同，索引定义不相同
   assert.tryThrow( SDB_IXM_EXIST, function()
   {
      maincl.createIndex( indexName1, { b: 1 } );
   } );
   checkNoTask( "Create index", csName, mainCLName, indexName1, SDB_IXM_EXIST );

   // 再次创建索引 b.索引名不同，索引定义相同
   assert.tryThrow( SDB_IXM_EXIST_COVERD_ONE, function()
   {
      maincl.createIndex( indexName2, { c: 1 } );
   } );
   checkNoTask( "Create index", csName, mainCLName, indexName2, SDB_IXM_EXIST_COVERD_ONE );

   // 再次创建索引 c.索引名和索引定义都相同
   assert.tryThrow( SDB_IXM_REDEF, function()
   {
      maincl.createIndex( indexName1, { c: 1 } );
   } );
   checkNoTask( "Create index", csName, mainCLName, indexName2, SDB_IXM_REDEF );

   //3.再次创建索引 d.索引名不同，索引定义部分相同
   maincl.createIndex( indexName2, { a: 1, c: 1 } );
   maincl.createIndex( indexName3, { c: -1 } );

   //4.检查任务信息,检查一致性
   var indexNames = [indexName1, indexName2, indexName3]
   checkIndexTask( "Create index", csName, mainCLName, indexNames, 0 );
   checkIndexTask( "Create index", csName, subCLName1, indexNames, 0 );
   checkIndexTask( "Create index", csName, subCLName2, indexNames, 0 );
   for( var i = 0; i < indexNames.length; i++ )
   {
      checkIndexExist( db, csName, mainCLName, indexNames[i], true );
      commCheckIndexConsistent( db, csName, subCLName1, indexNames[i], true );
      commCheckIndexConsistent( db, csName, subCLName2, indexNames[i], true );
   }

   //5.查询数据
   var actResult = maincl.find().sort( { a: 1 } ).hint( { "": indexName2 } );
   commCompareResults( actResult, docs );
   checkExplainByMaincl( maincl, { a: 5, c: 5 }, "ixscan", indexName2 );

   maincl.dropIndex( indexName1 );
   var actResult = maincl.find().sort( { a: 1 } ).hint( { "": indexName3 } );
   commCompareResults( actResult, docs );
   checkExplainByMaincl( maincl, { c: 5 }, "ixscan", indexName3 );

   //清除环境
   commDropCS( db, csName );
}

function checkNoTask ( taskTypeDesc, csName, clName, indexName, resultCode )
{
   var cursor = db.listTasks( { Name: csName + "." + clName, TaskTypeDesc: taskTypeDesc, IndexName: indexName, ResultCode: resultCode } );
   while( cursor.next() )
   {
      var taskInfo = cursor.current().toObj();
      throw new Error( "check task should be no exist! act task= " + JSON.stringify( taskInfo ) );
   }
   cursor.close();
}