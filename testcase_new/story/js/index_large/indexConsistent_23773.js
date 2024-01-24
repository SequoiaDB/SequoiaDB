/******************************************************************************
 * @Description   : seqDB-23773:创建多个索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.09.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_23773";
   var mainCLName = "maincl_23773";
   var subCLName1 = "subcl_23773_1";
   var subCLName2 = "subcl_23773_2";
   var indexName = "index_23773_";
   var recsNum = 62;

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 50 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 50 }, UpBound: { a: 100 } } );

   // 插入数据，每个索引插入100条数据
   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      for( var j = 0; j < 100; j++ )
      {
         var bson = {};
         bson["a" + i] = j;
         bson["a"] = j;
         docs.push( bson );
      }
   }
   maincl.insert( docs );

   // 创建大量索引
   var indexNames = [];
   var indexDef = new Object();
   for( var i = 0; i < recsNum; i++ )
   {
      indexDef["a" + i] = 1;
      maincl.createIndex( indexName + i, indexDef );
      delete indexDef["a" + i];
      indexNames.push( indexName + i );
   }

   // 校验任务和索引一致性
   checkIndexTask( "Create index", csName, mainCLName, indexNames, 0 );
   checkIndexTask( "Create index", csName, subCLName1, indexNames, 0 );
   checkIndexTask( "Create index", csName, subCLName2, indexNames, 0 );
   for( var i = 0; i < indexNames.length; i++ )
   {
      checkIndexExist( db, csName, mainCLName, indexNames[i], true );
      commCheckIndexConsistent( db, csName, subCLName1, indexNames[i], true );
      commCheckIndexConsistent( db, csName, subCLName2, indexNames[i], true );
   }

   // 随机匹配一个字段查询
   var key = parseInt( Math.random() * recsNum );
   var value = parseInt( Math.random() * 100 );
   var bson = {};
   bson["a" + key] = value;
   checkExplainByMaincl( maincl, bson, "ixscan", indexName + key );
   var actResult = maincl.find( bson );
   bson["a"] = value;
   var expResult = [bson];
   commCompareResults( actResult, expResult );

   // 再次创建一个索引
   assert.tryThrow( SDB_DMS_MAX_INDEX, function()
   {
      maincl.createIndex( indexName + 64, { b: 1 } );
   } );

   // 校验任务和索引一致性
   checkIndexTaskResult( "Create index", csName, mainCLName, indexName + 64, SDB_DMS_MAX_INDEX );
   checkIndexTaskResult( "Create index", csName, subCLName1, indexName + 64, SDB_DMS_MAX_INDEX );
   checkIndexTaskResult( "Create index", csName, subCLName2, indexName + 64, SDB_DMS_MAX_INDEX );
   checkIndexExist( db, csName, mainCLName, indexName + 64, false );
   commCheckIndexConsistent( db, csName, subCLName1, indexName + 64, false );
   commCheckIndexConsistent( db, csName, subCLName2, indexName + 64, false );

   //清除环境
   commDropCS( db, csName );
}