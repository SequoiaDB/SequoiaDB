/************************************
*@Description: seqDB-8011:匹配不到记录，upsert使用push_all更新符更新数组对象
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/
testConf.clName = COMMCLNAME + "_push_all8011";
main( test );

function test ( testPara )
{
   //insert object
   var doc = { a: 1 };
   testPara.testCL.insert( doc );

   //upsert any object when match nothing,use matches and
   var upsertCondition1 = {
      $push_all: {
         object1: [200, 4, "string"], object2: [10, false, 55, 70], object3: [102, 25, 80],
         object4: [2, 3, 1],
         "object5.1": [30, 50, 40, 50], "object6.1": [50, 99, 30], "object7.1": [20, 99, 30],
         object8: [11]
      }
   };
   var findCondition1 = {
      $and: [{ object1: [200, [30, 50, [90, 40], 80], 4, 40, "string"] },
      { object2: [5, 7, 9, 55, -8, 10] },
      { object3: [35, 88, 46] },
      { object4: { $et: { $decimal: "2" } } },
      { object5: { $all: [50, [30, 50, [90, 40], 80], 25, 40, 15] } },
      { object6: { $all: [50, [30, 50, [90, 40], 80], 25, 40, 15] } },
      { object7: { $all: [50, [30, 50, [90, 40], 80], 25, 40, 15] } },
      { object9: [5, [9, 8, 4, 3], 10] },
      { c: { $gt: 100 } }]
   };
   testPara.testCL.upsert( upsertCondition1, findCondition1 );

   //check result
   var expRecs1 = [{
      object1: [200, [30, 50, [90, 40], 80], 4, 40, "string", 200, 4, "string"],
      object2: [5, 7, 9, 55, -8, 10, 10, false, 55, 70],
      object3: [35, 88, 46, 102, 25, 80],
      object4: { $decimal: "2" },
      object5: [50, [30, 50, [90, 40], 80, 30, 50, 40, 50], 25, 40, 15],
      object6: [50, [30, 50, [90, 40], 80, 50, 99, 30], 25, 40, 15],
      object7: [50, [30, 50, [90, 40], 80, 20, 99, 30], 25, 40, 15],
      object8: [11],
      object9: [5, [9, 8, 4, 3], 10]
   },
   { a: 1 }];
   checkResult( testPara.testCL, null, null, expRecs1, { a: 1 } );

   //upsert any object when match nothing,use matches or
   var upsertCondition2 = {
      $push_all: {
         object1: [200, 4, "string"], object2: [10, false, 55, 70], object3: [102, 25, 80],
         object4: [2, 3, 1],
         "object5.1": [30, 50, 40, 50], "object6.1": [50, 99, 30], "object7.1": [20, 99, 30],
         object8: [11]
      }
   };
   var findCondition2 = {
      $or: [{ object1: [10, 13, 15] },
      { object4: { $et: { $decimal: "22" } } },
      { c: { $gt: 100 } }]
   };
   testPara.testCL.upsert( upsertCondition2, findCondition2 );

   //check result
   var expRecs2 = [{
      object1: [200, [30, 50, [90, 40], 80], 4, 40, "string", 200, 4, "string"],
      object2: [5, 7, 9, 55, -8, 10, 10, false, 55, 70],
      object3: [35, 88, 46, 102, 25, 80],
      object4: { $decimal: "2" },
      object5: [50, [30, 50, [90, 40], 80, 30, 50, 40, 50], 25, 40, 15],
      object6: [50, [30, 50, [90, 40], 80, 50, 99, 30], 25, 40, 15],
      object7: [50, [30, 50, [90, 40], 80, 20, 99, 30], 25, 40, 15],
      object8: [11],
      object9: [5, [9, 8, 4, 3], 10]
   },
   {
      object1: [200, 4, "string"], object2: [10, false, 55, 70], object3: [102, 25, 80],
      object4: [2, 3, 1],
      object5: { 1: [30, 50, 40, 50] }, object6: { 1: [50, 99, 30] }, object7: { 1: [20, 99, 30] },
      object8: [11]
   },
   { a: 1 }];
   checkResult( testPara.testCL, null, null, expRecs2, { a: 1 } );

   //delete all data
   testPara.testCL.remove();

   //upsert any object when match nothing,use matches not
   var upsertCondition3 = {
      $push_all: {
         object1: [200, 4, "string"], object2: [10, false, 55, 70], object3: [102, 25, 80],
         object4: [2, 3, 1],
         "object5.1": [30, 50, 40, 50], "object6.1": [50, 99, 30], "object7.1": [20, 99, 30],
         object8: [11]
      }
   };
   var findCondition3 = {
      $not: [{ object1: [10, 13, 15] },
      { object4: { $et: { $decimal: "22" } } },
      { c: { $gt: 100 } }]
   };
   testPara.testCL.upsert( upsertCondition3, findCondition3 );

   //check result
   var expRecs3 = [{
      object1: [200, 4, "string"], object2: [10, false, 55, 70], object3: [102, 25, 80],
      object4: [2, 3, 1],
      object5: { 1: [30, 50, 40, 50] }, object6: { 1: [50, 99, 30] }, object7: { 1: [20, 99, 30] },
      object8: [11]
   }];
   checkResult( testPara.testCL, null, null, expRecs3, { a: 1 } );
}

