/******************************************************************************
 * @Description   : seqDB-23229 :: 单键索引且NotArray:false，增删改查索引数组字段数据
 * @Author        : Yu Fan
 * @CreateTime    : 2021.01.08
 * @LastEditTime  : 2021.01.11
 * @LastEditors   : Yu Fan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_23229";
var indexName = "Index_23229";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;

   // 创建索引
   cl.createIndex( indexName, { a: 1 }, { NotArray: false } );

   // 检查索引
   var indexInfo = JSON.parse( cl.getIndex( indexName ).toString() );
   assert.equal( indexInfo.IndexDef.NotArray, false, "indexInfo = " + indexInfo );

   // 增删改查索引非数组数据
   cl.insert( { a: 1, b: 1, c: 1 } );
   var cursor = cl.find( { a: 1 } )
   commCompareResults( cursor, [{ a: 1, b: 1, c: 1 }] );

   cl.update( { $set: { a: 2 } }, { a: 1 } );
   cursor = cl.find( { a: 2 } )
   commCompareResults( cursor, [{ a: 2, b: 1, c: 1 }] );

   cl.remove( { a: 2 } );
   assert.equal( cl.count(), 0 );

   // 增删改查索引数组数据
   cl.insert( { a: [6, 7, 8] } );
   cursor = cl.find( { a: 6 } )
   commCompareResults( cursor, [{ a: [6, 7, 8] }] );

   cl.update( { $set: { a: [9, 10] } }, { a: 6 } );
   cursor = cl.find( { a: [9, 10] } );
   commCompareResults( cursor, [{ a: [9, 10] }] );

   cl.remove( { a: [9, 10] } )
   assert.equal( cl.count(), 0 );

   // 插入索引非数组数据，更新索引字段为数组
   cl.insert( { a: 11, b: 11, c: 11 } );
   cl.update( { $set: { a: [12, 13, 14] } }, { a: 11 } )
   cursor = cl.find( { a: [12, 13, 14] } );
   commCompareResults( cursor, [{ a: [12, 13, 14], b: 11, c: 11 }] );

   // 插入索引字段为数组，更新为非数组
   cl.insert( { a: [15, 16, 17], b: 12, c: 12 } );
   cl.update( { $set: { a: 15 } }, { a: [15, 16, 17] } )
   cursor = cl.find( { a: 15 } );
   commCompareResults( cursor, [{ a: 15, b: 12, c: 12 }] );
   cl.remove();

   // 增删改查非索引字段数组、非数组数据；
   cl.insert( { c: 7, d: 7 } );
   cl.update( { $set: { c: [8, 9, 10] } }, { c: 7 } );
   cursor = cl.find( { c: [8, 9, 10] } );
   commCompareResults( cursor, [{ c: [8, 9, 10], d: 7 }] );
   cl.remove( { d: 7 } );
   assert.equal( cl.count(), 0 );

   cl.insert( { c: [1, 2, 3], d: [1, 2, 3] } );
   cl.update( { $set: { d: 1 } }, { d: [1, 2, 3] } );
   cursor = cl.find( { c: [1, 2, 3] } );
   commCompareResults( cursor, [{ c: [1, 2, 3], d: 1 }] );
   cl.remove( { c: [1, 2, 3] } );
   assert.equal( cl.count(), 0 );
}

