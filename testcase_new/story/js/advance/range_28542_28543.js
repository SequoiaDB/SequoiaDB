/******************************************************************************
 * @Description   : seqDB-28542:普通表不带sort,hint指定索引，使用$Range操作符点查后使用count
 *                : seqDB-28543:普通表不带sort,hint指定索引，使用$Range操作符范围查后使用count
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.10.31
 * @LastEditTime  : 2022.11.01
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_28542_28543";

main( test );

function test ( args )
{
   var dbcl = args.testCL;
   var idxName = "idx_28542_28543";
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

   // 指定hint，不指定sort批量点查
   var result = dbcl.find().hint( {
      "": idxName,
      "$Range": {
         "IsAllEqual": true,
         "PrefixNum": 2,
         "IndexValue": [{ "1": 1, "2": 1 }, { "1": 3, "2": 3 }]
      }
   } ).count();
   var expResult = 2;
   assert.equal( result, expResult );

   // 指定hint，不指定sort批量范围查
   var result = dbcl.find().hint( {
      "": idxName,
      "$Range": {
         "IsAllEqual": false,
         "PrefixNum": [2, 2],
         "IndexValueIncluded": [[true, false], [true, false]],
         "IndexValue": [[{ "1": 2, "2": 2 }, { "1": 2, "2": 6 }], [{ "1": 3, "2": 2 }, { "1": 3, "2": 6 }]]
      }
   } ).count();
   var expResult = [];
   for( var i = 2; i <= 3; i++ )
   {
      for( var j = 2; j < 6; j++ )
      {
         expResult.push( { a: i, b: j } );
      }
   }
   assert.equal( result, expResult.length );
}