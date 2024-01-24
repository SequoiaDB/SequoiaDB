/******************************************************************************
 * @Description   : seqDB-23755:创建索引后执行CRUD操作
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.09.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_23755";

main( test );
function test ( testPara )
{
   var indexName = "index_23755";
   var recsNum = 300;
   var removeNum = 100;

   var expResult = [];
   var cl = testPara.testCL;
   cl.createIndex( indexName, { "a": 1 } );

   //1.插入数据
   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { "a": i, "b": i } );
   }
   cl.insert( docs );

   //2.更新数据
   cl.update( { $inc: { "a": 1 } } );
   for( var i = 0; i < recsNum; i++ )
   {
      expResult.push( { "a": i + 1, "b": i } );
   }
   commCompareResults( cl.find().sort( { a: 1 } ), expResult );

   //3.删除部分数据
   for( var i = 1; i < removeNum + 1; i++ )
   {
      cl.remove( { "a": i } )
   }
   expResult = [];
   for( var i = removeNum; i < recsNum; i++ )
   {
      expResult.push( { "a": i + 1, "b": i } );
   }
   commCompareResults( cl.find().sort( { a: 1 } ), expResult );

   //4.执行count查询数据
   assert.equal( cl.count(), recsNum - removeNum );

   //5.findAndModify
   cl.find().update( { $inc: { "a": 1 } } ).toArray();
   expResult = [];
   for( var i = removeNum; i < recsNum; i++ )
   {
      expResult.push( { "a": i + 2, "b": i } );
   }
   commCompareResults( cl.find().sort( { a: 1 } ), expResult );

   //6.findAndRemove
   cusor = cl.find( { "a": { $lte: 300 } } ).remove().toArray();
   expResult = [{ "a": 301, "b": 299 }];
   commCompareResults( cl.find().sort( { a: 1 } ), expResult );

   //7.truncate
   cl.truncate();
   expResult = [];
   commCompareResults( cl.find().sort( { a: 1 } ), expResult );
}