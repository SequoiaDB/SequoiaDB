/******************************************************************************
 * @Description   : seqDB-25673~25676
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2022.04.05
 * @LastEditTime  : 2022.04.05
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_range_25673";

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

   // seqDB-25673:逆序复合索引，sort排序正序，PrefixNum包含多个不同取值，IndexValue区间之间乱序
   cl.createIndex( idxName, { a: -1, b: -1, c: -1, d: -1 } );
   // IndexValue区间之间正序（跟乱序结果做对比）
   var cursor = cl.find().sort( { "a": 1, "b": 1, "c": 1, d: 1 } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": [[true, true], [true, false], [false, true], [false, false]],
         "PrefixNum": [4, 2, 1, 3],
         "IndexValue": [
            [{ "a": 0, "b": 0 }, { "a": 2, "b": 2 }],
            [{ "a": 5, "b": 5, "c": 5 }, { "a": 7, "b": 7, "c": 7 }],
            [{ "a": 100, "b": 100, "c": 100, "d": 100 }, { "a": 102, "b": 102, "c": 102, "d": 102 }],
            [{ "a": 997 }, { "a": 999 }]]
      }
   } );
   commCompareResults( cursor, [{ "a": 0, "b": 0, "c": 0 }, { "a": 1, "b": 1, "c": 1 }, { "a": 2, "b": 2, "c": 2 }, { "a": 5, "b": 5, "c": 5 }, { "a": 6, "b": 6, "c": 6 }, { "a": 101, "b": 101, "c": 101 }, { "a": 102, "b": 102, "c": 102 }, { "a": 997, "b": 997, "c": 997 }, { "a": 998, "b": 998, "c": 998 }, { "a": 999, "b": 999, "c": 999 }] );
   // IndexValue区间之间乱序
   var cursor = cl.find().sort( { "a": 1, "b": 1, "c": 1, d: 1 } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": [[true, true], [true, false], [false, true], [false, false]],
         "PrefixNum": [4, 2, 1, 3],
         "IndexValue": [
            [{ "a": 0, "b": 0 }, { "a": 2, "b": 2 }],
            [{ "a": 100, "b": 100, "c": 100, "d": 100 }, { "a": 102, "b": 102, "c": 102, "d": 102 }],
            [{ "a": 5, "b": 5, "c": 5 }, { "a": 7, "b": 7, "c": 7 }],
            [{ "a": 997 }, { "a": 999 }]]
      }
   } );
   commCompareResults( cursor, [{ "a": 0, "b": 0, "c": 0 }, { "a": 1, "b": 1, "c": 1 }, { "a": 2, "b": 2, "c": 2 }, { "a": 6, "b": 6, "c": 6 }, { "a": 7, "b": 7, "c": 7 }, { "a": 100, "b": 100, "c": 100 }, { "a": 101, "b": 101, "c": 101 }, { "a": 997, "b": 997, "c": 997 }, { "a": 998, "b": 998, "c": 998 }, { "a": 999, "b": 999, "c": 999 }] );
   cl.dropIndex( idxName );

   // seqDB-25674:正逆序复合索引，sort排序混合正逆序，IndexValue区间范围乱序
   cl.createIndex( idxName, { a: 1, b: -1, c: 1 } );
   // sort排序混合正逆序，跟索引排序完全相同
   var cursor = cl.find().sort( { "a": 1, "b": -1, "c": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": [[true, true], [true, false], [false, true], [false, false]],
         "PrefixNum": [3, 3, 3, 3],
         "IndexValue": [
            [{ "a": 0, "b": 0, "c": 0 }, { "a": 2, "b": 2, "c": 2 }],
            [{ "a": 100, "b": 100, "c": 100 }, { "a": 102, "b": 102, "c": 102 }],
            [{ "a": 997, "b": 997, "c": 997 }, { "a": 999, "b": 999, "c": 999 }],
            [{ "a": 5, "b": 5, "c": 5 }, { "a": 7, "b": 7, "c": 7 }]]
      }
   } );
   commCompareResults( cursor, [{ "a": 0, "b": 0, "c": 0 }, { "a": 1, "b": 1, "c": 1 }, { "a": 2, "b": 2, "c": 2 }, { "a": 6, "b": 6, "c": 6 }, { "a": 100, "b": 100, "c": 100 }, { "a": 101, "b": 101, "c": 101 }, { "a": 998, "b": 998, "c": 998 }, { "a": 999, "b": 999, "c": 999 }] );
   // sort排序混合正逆序，跟索引排序完全不同   
   var cursor = cl.find().sort( { "a": -1, "b": 1, "c": -1 } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": [[true, true], [true, false], [false, true], [false, false]],
         "PrefixNum": [3, 3, 3, 3],
         "IndexValue": [
            [{ "a": 2, "b": 2, "c": 2 }, { "a": 0, "b": 0, "c": 0 }],
            [{ "a": 102, "b": 102, "c": 102 }, { "a": 100, "b": 100, "c": 100 }],
            [{ "a": 999, "b": 999, "c": 999 }, { "a": 997, "b": 997, "c": 997 }],
            [{ "a": 7, "b": 7, "c": 7 }, { "a": 5, "b": 5, "c": 5 }]]
      }
   } );
   commCompareResults( cursor, [{ "a": 998, "b": 998, "c": 998 }, { "a": 997, "b": 997, "c": 997 }, { "a": 102, "b": 102, "c": 102 }, { "a": 101, "b": 101, "c": 101 }, { "a": 6, "b": 6, "c": 6 }, { "a": 2, "b": 2, "c": 2 }, { "a": 1, "b": 1, "c": 1 }, { "a": 0, "b": 0, "c": 0 }] );
   // 单个范围区间左右边界跟排序相反
   var cursor = cl.find().sort( { "a": -1, "b": 1, "c": -1 } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": [[true, true], [true, false], [false, true], [false, false]],
         "PrefixNum": [3, 3, 3, 3],
         "IndexValue": [
            [{ "a": 0, "b": 0, "c": 0 }, { "a": 2, "b": 2, "c": 2 }],
            [{ "a": 100, "b": 100, "c": 100 }, { "a": 102, "b": 102, "c": 102 }],
            [{ "a": 997, "b": 997, "c": 997 }, { "a": 999, "b": 999, "c": 999 }],
            [{ "a": 5, "b": 5, "c": 5 }, { "a": 7, "b": 7, "c": 7 }]]
      }
   } );
   try
   {
      cursor.next();
      throw new Error( "expect fail but actual success." );
   }
   catch( e )
   {
      assert.equal( e, -6 );
   }
   cl.dropIndex( idxName );

   // seqDB-25675:sort排序正序，startValue > endValue
   cl.createIndex( idxName, { a: 1, b: 1, c: 1 } );
   var cursor = cl.find().sort( { "a": 1, "b": 1, "c": 1 } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": [[true, true]],
         "PrefixNum": [3],
         "IndexValue": [
            [{ "a": 10, "b": 10, "c": 10 }, { "a": 2, "b": 2, "c": 2 }]]
      }
   } );
   try
   {
      cursor.next();
      throw new Error( "expect fail but actual success." );
   }
   catch( e )
   {
      assert.equal( e, -6 );
   }
   cl.dropIndex( idxName );

   // seqDB-25676:sort排序逆序，startValue < endValue
   cl.createIndex( idxName, { a: 1, b: 1, c: 1 } );
   var cursor = cl.find().sort( { "a": -1, "b": -1, "c": -1 } ).hint( {
      "$Range": {
         "IsAllEqual": false,
         "IndexValueIncluded": [[true, true]],
         "PrefixNum": [3],
         "IndexValue": [
            [{ "a": 0, "b": 0, "c": 0 }, { "a": 2, "b": 2, "c": 2 }]]
      }
   } );
   try
   {
      cursor.next();
      throw new Error( "expect fail but actual success." );
   }
   catch( e )
   {
      assert.equal( e, -6 );
   }
   cl.dropIndex( idxName );
}