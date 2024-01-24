/******************************************************************************
 * @Description   : seqDB-25688:$Position及advance操作符基本功能验证
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2022.04.05
 * @LastEditTime  : 2022.04.06
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_range_25688";

main( test );

function test ( args )
{
   var cl = args.testCL;
   var idxName = "idx";
   var recsNum = 1000;

   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      docs.push( { a: i, b: i, c: i } );
   }
   cl.insert( docs );
   cl.createIndex( idxName, { a: 1, b: 1, c: 1 } );
   var cursor = cl.find().sort( { "a": 1, b: 1, c: 1 } ).hint( { "$Position": { "IndexValue": { "a": 996, "b": 996, "c": 996 }, "Type": 2, "PrefixNum": 2 } } );
   commCompareResults( cursor, [{ "a": 997, "b": 997, "c": 997 }, { "a": 998, "b": 998, "c": 998 }, { "a": 999, "b": 999, "c": 999 }] );

   // advance接口
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1, "b": 1, "c": 1 } );
   assert.equal( cursor.next().toObj(), { "a": 0, "b": 0, "c": 0 } );

   cursor._cursor.advance( { "IndexValue": { "a": 996, "b": 996, "c": 996 }, "Type": 1, "PrefixNum": 3 } );
   assert.equal( cursor.next().toObj(), { "a": 996, "b": 996, "c": 996 } );

   cursor.close();
}