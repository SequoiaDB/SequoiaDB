/******************************************************************************
 * @Description   : seqDB-27389:普通表不带sort，hint指定索引，使用$Range操作符点查
 *                : seqDB-27390:普通表不带sort，hint指定索引，使用$Range操作批量查
 * @Author        : liuli
 * @CreateTime    : 2022.09.03
 * @LastEditTime  : 2022.09.03
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_27389_27390";

main( test );

function test ( args )
{
   var dbcl = args.testCL;
   var idxName = "idx_27389_27390";
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

   // 指定hint不指定sort批量点查
   var cursor = dbcl.find().hint( {
      "": idxName,
      "$Range": {
         "IsAllEqual": true,
         "PrefixNum": 2,
         "IndexValue": [{ "1": 1, "2": 1 }, { "1": 3, "2": 3 }]
      }
   } );
   var expResult = [{ a: 1, b: 1 }, { a: 3, b: 3 }];
   commCompareResults( cursor, expResult );

   // 指定hint不指定sort批量范围查
   var cursor = dbcl.find().hint( {
      "": idxName,
      "$Range": {
         "IsAllEqual": false,
         "PrefixNum": [2, 2],
         "IndexValueIncluded": [[true, false], [true, false]],
         "IndexValue": [[{ "1": 2, "2": 2 }, { "1": 2, "2": 6 }], [{ "1": 3, "2": 2 }, { "1": 3, "2": 6 }]]
      }
   } );
   var expResult = [];
   for( var i = 2; i <= 3; i++ )
   {
      for( var j = 2; j < 6; j++ )
      {
         expResult.push( { a: i, b: j } );
      }
   }
   commCompareResults( cursor, expResult );
}