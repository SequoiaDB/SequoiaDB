/******************************************************************************
 * @Description   : seqDB-26597:游标advance，IndexValue指定嵌套字段
 * @Author        : liuli
 * @CreateTime    : 2022.06.02
 * @LastEditTime  : 2022.11.28
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_26597";

main( test );

function test ( args )
{
   var cl = args.testCL;
   var indexName = "index_26597";
   var recsNum = 1000;

   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { a: { b: i, c: i } } );
   }
   cl.insert( docs );

   // 指定"a.b"创建索引
   cl.createIndex( indexName, { "a.b": 1 } );

   var expResult = { a: { b: 300, c: 300 } };
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a.b": 1 } ).hint( { "": indexName } );
   cursor.next();

   // IndexValue指定嵌套字段{ "a.b": 300 }
   cursor._cursor.advance( { "IndexValue": { "a.b": 300 }, "Type": 1, "PrefixNum": 1 } );
   assert.equal( cursor.next().toObj(), expResult );

   cursor.close();

   // IndexValue指定非嵌套字段{ "a": { "b": 300 } }
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a.b": 1 } ).hint( { "": indexName } );
   cursor.next();
   cursor._cursor.advance( { "IndexValue": { "a": { "b": 300 } }, "Type": 1, "PrefixNum": 1 } );
   assert.equal( cursor.next().toObj(), expResult );

   cursor.close();
}