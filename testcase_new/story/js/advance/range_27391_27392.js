/******************************************************************************
 * @Description   : seqDB-27391:主表不带sort，hint指定索引，使用$Range操作符点查
 *                : seqDB-27392:主表不带sort，hint指定索引，使用$Range操作批量查
 * @Author        : liuli
 * @CreateTime    : 2022.09.03
 * @LastEditTime  : 2022.09.05
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var csName = "cs_27391_27392";
   var mainCLName = "mainCL_27391_27392";
   var subCLName1 = "subCL_27391_27392_1";
   var subCLName2 = "subCL_27391_27392_2";
   var idxName = "idx_27391_27392";
   var recsNum = 10;

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 5 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 5 }, UpBound: { a: 10 } } );

   var docs = [];
   for( var i = 0; i < recsNum; i++ )
   {
      for( var j = 0; j < recsNum; j++ )
      {
         docs.push( { a: i, b: j } );
      }
   }
   maincl.insert( docs );
   maincl.createIndex( idxName, { a: 1, b: 1 } );

   // 指定hint不指定sort批量点查
   var cursor = maincl.find().hint( {
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
   var cursor = maincl.find().hint( {
      "": idxName,
      "$Range": {
         "IsAllEqual": false,
         "PrefixNum": [2, 2, 2, 2],
         "IndexValueIncluded": [[true, false], [true, false], [true, false], [true, false]],
         "IndexValue": [[{ "1": 4, "2": 2 }, { "1": 4, "2": 6 }], [{ "1": 5, "2": 2 }, { "1": 5, "2": 6 }], [{ "1": 5, "2": 2 }, { "1": 5, "2": 6 }], [{ "1": 6, "2": 2 }, { "1": 6, "2": 6 }]]
      }
   } );
   var expResult = [];
   for( var i = 4; i <= 6; i++ )
   {
      for( var j = 2; j < 6; j++ )
      {
         expResult.push( { a: i, b: j } );
      }
   }

   var actResult = [];
   while( cursor.next() )
   {
      var actObj = cursor.current().toObj();
      delete actObj._id;
      actResult.push( actObj );
   }
   cursor.close();
   assert.equal( actResult.sort( sortBy( 'a' ) ), expResult.sort( sortBy( 'a' ) ) );

   commDropCS( db, csName );
}

function sortBy ( field )
{
   return function( a, b )
   {
      return a[field] > b[field];
   }
}