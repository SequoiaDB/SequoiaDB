/******************************************************************************
 * @Description   : seqDB-24373:创建本地索引后执行数据操作 
 * @Author        : liuli
 * @CreateTime    : 2021.09.27
 * @LastEditTime  : 2021.09.30
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_24373";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ( args )
{
   var dbcl = args.testCL;
   var recsNum = 63;
   var indexName = "index_24373";
   var recsNum = 300;
   var removeNum = 100;

   var expResult = [];
   var nodeName = getCLOneNodeName( db, COMMCSNAME + "." + testConf.clName );
   dbcl.createIndex( indexName, { a: 1 }, { Standalone: true }, { NodeName: nodeName } );

   //1.插入数据
   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { "a": i, "b": i } );
   }
   dbcl.insert( docs );

   //2.更新数据
   dbcl.update( { $inc: { "a": 1 } } );
   for( var i = 0; i < recsNum; i++ )
   {
      expResult.push( { "a": i + 1, "b": i } );
   }
   commCompareResults( dbcl.find().sort( { a: 1 } ), expResult );

   //3.删除部分数据
   for( var i = 1; i < removeNum + 1; i++ )
   {
      dbcl.remove( { "a": i } )
   }
   expResult = [];
   for( var i = removeNum; i < recsNum; i++ )
   {
      expResult.push( { "a": i + 1, "b": i } );
   }
   commCompareResults( dbcl.find().sort( { a: 1 } ), expResult );

   //4.执行count查询数据
   assert.equal( dbcl.count(), recsNum - removeNum );

   //5.findAndModify
   dbcl.find().update( { $inc: { "a": 1 } } ).toArray();
   expResult = [];
   for( var i = removeNum; i < recsNum; i++ )
   {
      expResult.push( { "a": i + 2, "b": i } );
   }
   commCompareResults( dbcl.find().sort( { a: 1 } ), expResult );

   //6.findAndRemove
   cusor = dbcl.find( { "a": { $lte: 300 } } ).remove().toArray();
   expResult = [{ "a": 301, "b": 299 }];
   commCompareResults( dbcl.find().sort( { a: 1 } ), expResult );

   //7.truncate
   dbcl.truncate();
   expResult = [];
   commCompareResults( dbcl.find().sort( { a: 1 } ), expResult );
}