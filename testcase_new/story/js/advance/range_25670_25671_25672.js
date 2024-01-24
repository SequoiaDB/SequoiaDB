/******************************************************************************
 * @Description   : seqDB-25670~25672
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2022.04.05
 * @LastEditTime  : 2022.04.05
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_range_25670";

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

   // seqDB-25670:逆序索引，sort排序正序，IndexValue多个区间且区间排序逆序
   cl.createIndex( idxName, { a: 1, b: 1, c: 1 } );
   var cursor = cl.find().sort( { "a": 1, "b": 1, "c": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": true,
         "PrefixNum": 3,
         "IndexValue": [{ "a": 1000, "b": 1000, "c": 1000 }, { "a": 5, "b": 5, "c": 5 }, { "a": 3, "b": 3, "c": 3 }]
      }
   } );
   commCompareResults( cursor, [{ "a": 3, "b": 3, "c": 3 }, { "a": 5, "b": 5, "c": 5 }] );
   cl.dropIndex( idxName );

   // seqDB-25671:正逆序复合索引，sort排序为正逆序且跟索引排序不同，IndexValue多个区间且区间排序乱序
   cl.createIndex( idxName, { a: 1, b: -1, c: 1 } );
   // sort排序跟索引字段排序相同(SQL端覆盖，此处自动化简单覆盖基本场景)
   var cursor = cl.find().sort( { "a": 1, "b": -1, "c": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": true,
         "PrefixNum": 3,
         "IndexValue": [{ "a": 5, "b": 5, "c": 5 }, { "a": 1000, "b": 1000, "c": 1000 }, { "a": 3, "b": 3, "c": 3 }]
      }
   } );
   commCompareResults( cursor, [{ "a": 3, "b": 3, "c": 3 }, { "a": 5, "b": 5, "c": 5 }] );
   // sort排序跟索引字段排序不同
   var cursor = cl.find().sort( { "a": -1, "b": 1, "c": -1 } ).hint( {
      "$Range": {
         "IsAllEqual": true,
         "PrefixNum": 3,
         "IndexValue": [{ "a": 5, "b": 5, "c": 5 }, { "a": 1000, "b": 1000, "c": 1000 }, { "a": 3, "b": 3, "c": 3 }]
      }
   } );
   commCompareResults( cursor, [{ "a": 5, "b": 5, "c": 5 }, { "a": 3, "b": 3, "c": 3 }] );
   cl.dropIndex( idxName );

   // seqDB-25672:正序复合索引，sort排序部分逆序
   cl.createIndex( idxName, { a: 1, b: 1, c: 1 } );
   var cursor = cl.find().sort( { "a": 1, "b": -1, "c": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": true,
         "PrefixNum": 3,
         "IndexValue": [{ "a": 5, "b": 5, "c": 5 }, { "a": 1000, "b": 1000, "c": 1000 }, { "a": 3, "b": 3, "c": 3 }]
      }
   } );
   try
   {
      cursor.next();
      throw new Error( "expect fail but actual success." );
   }
   catch( e )
   {
      assert.equal( e, -32 );
   }
   cl.dropIndex( idxName );
}