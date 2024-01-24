/******************************************************************************
 * @Description   : seqDB-23774:创建索引后执行CRUD操作
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.09.02
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_23774";
   var mainCLName = "maincl_23774";
   var subCLName1 = "subcl_23774_1";
   var subCLName2 = "subcl_23774_2";
   var indexName = "index_23774";
   var recsNum = 300;
   var removeNum = 100;

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { IsMainCL: true, ShardingKey: { b: 1 }, ShardingType: "range" } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { b: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { b: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { b: 0 }, UpBound: { b: 200 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { b: 200 }, UpBound: { b: 400 } } );

   var expResult = [];
   maincl.createIndex( indexName, { "a": 1 } );

   //1.插入数据
   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { "a": i, "b": i } );
   }
   maincl.insert( docs );

   //2.更新数据
   maincl.update( { $inc: { "a": 1 } } );
   for( var i = 0; i < recsNum; i++ )
   {
      expResult.push( { "a": i + 1, "b": i } );
   }
   commCompareResults( maincl.find().sort( { a: 1 } ), expResult );

   //3.删除部分数据
   for( var i = 1; i < removeNum + 1; i++ )
   {
      maincl.remove( { "a": i } )
   }
   expResult = [];
   for( var i = removeNum; i < recsNum; i++ )
   {
      expResult.push( { "a": i + 1, "b": i } );
   }
   commCompareResults( maincl.find().sort( { a: 1 } ), expResult );

   //4.执行count查询数据
   assert.equal( maincl.count(), recsNum - removeNum );

   //5.findAndModify
   maincl.find().update( { $inc: { "a": 1 } } ).toArray();
   expResult = [];
   for( var i = removeNum; i < recsNum; i++ )
   {
      expResult.push( { "a": i + 2, "b": i } );
   }
   commCompareResults( maincl.find().sort( { a: 1 } ), expResult );

   //6.findAndRemove
   cusor = maincl.find( { "a": { $lte: 300 } } ).remove().toArray();
   expResult = [{ "a": 301, "b": 299 }];
   commCompareResults( maincl.find().sort( { a: 1 } ), expResult );

   //7.truncate
   maincl.truncate();
   expResult = [];
   commCompareResults( maincl.find().sort( { a: 1 } ), expResult );

   commDropCS( db, csName );
}