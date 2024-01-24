/******************************************************************************
 * @Description   : seqDB-28548:普通表输入查询匹配不到的数据，指定hint使用$Range操作后使用count
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.10.31
 * @LastEditTime  : 2022.11.01
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_28548";

main( test );

function test ( args )
{
   var dbcl = args.testCL;
   var idxName = "idx_28548";
   var recsNum = 10;

   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      for( var j = 0; j < recsNum; j++ )
      {
         docs.push( { a: i, b: j } );
      }
   }
   dbcl.insert( docs );
   dbcl.createIndex( idxName, { a: 1, b: 1 } );

   // hint指定索引，输入查询匹配不到的数据，使用$Range操作符点查
   var result = dbcl.count( { a: 1, b: 2 } ).hint( {
      "": idxName,
      "$Range": {
         "IsAllEqual": true,
         "PrefixNum": 2,
         "IndexValue": [{ "1": 1, "2": 1 }, { "1": 3, "2": 3 }]
      }
   } );
   var expResult = 0;
   assert.equal( result, expResult );

   // hint指定索引,输入查询匹配不到的数据，使用$Range操作符范围查
   var result = dbcl.count( { a: 1, b: 2 } ).hint( {
      "": idxName,
      "$Range": {
         "IsAllEqual": false,
         "PrefixNum": [2, 2],
         "IndexValueIncluded": [[true, false], [true, false]],
         "IndexValue": [[{ "1": 2, "2": 2 }, { "1": 2, "2": 6 }], [{ "1": 3, "2": 2 }, { "1": 3, "2": 6 }]]
      }
   } );
   var expResult = 0;
   assert.equal( result, expResult );
}