/************************************
*@Description: seqDB-8014:匹配不到记录，upsert使用replace更新符更新任意类型对象
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/
testConf.clName = COMMCLNAME + "_replace8014";
main( test );

function test ( testPara )
{
   //insert object
   var doc = { a: 1 };
   testPara.testCL.insert( doc );

   //upsert object when match nothing,use matches and
   var upsertCondition1 = { $replace: { object1: 123, object2: [10, false, 55, 70] } };
   var findCondition1 = {
      $and: [{ object1: 456 },
      { object2: [5, 7, 9, 55, -8, 10] },
      { object4: { $et: { $decimal: "2" } } },
      { object5: { $all: [50, [30, 50, [90, 40], 80], 25, 40, 15] } },
      { c: { $gt: 100 } }]
   };
   testPara.testCL.upsert( upsertCondition1, findCondition1 );

   //check result
   var expRecs1 = [{ object1: 123, object2: [10, false, 55, 70] },
   { a: 1 }];
   checkResult( testPara.testCL, null, null, expRecs1, { a: 1 } );

   //upsert any object when match nothing,use matches or
   var upsertCondition2 = { $replace: { object1: 56, object2: [null, 70] } };
   var findCondition2 = {
      $or: [{ object1: [10, 13, 15] },
      { object4: { $et: { $decimal: "22" } } },
      { c: { $gt: 100 } }]
   };
   testPara.testCL.upsert( upsertCondition2, findCondition2 );

   //check result
   var expRecs2 = [{ object1: 123, object2: [10, false, 55, 70] },
   { object1: 56, object2: [null, 70] },
   { a: 1 }];
   checkResult( testPara.testCL, null, null, expRecs2, { a: 1 } );

   //delete all data
   testPara.testCL.remove();

   //upsert any object when match nothing,use matches not
   var upsertCondition3 = { $replace: { object1: 56, object2: [null, 70] } };
   var findCondition3 = {
      $not: [{ object1: [10, 13, 15] },
      { object4: { $et: { $decimal: "22" } } },
      { c: { $gt: 100 } }]
   };
   testPara.testCL.upsert( upsertCondition3, findCondition3 );

   //check result
   var expRecs3 = [{ object1: 56, object2: [null, 70] }];
   checkResult( testPara.testCL, null, null, expRecs3, { a: 1 } );
}

