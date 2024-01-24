/************************************
*@Description: seqDB-8006:update使用push更新符更新不存在的数组对象
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/
testConf.clName = COMMCLNAME + "_push8006";
main( test );

function test ( testPara )
{
   //insert data
   var doc1 = [{ object0: [10, -30, 20] }];
   testPara.testCL.insert( doc1 );

   //update use push object does not exist,no matches
   var updateCondition1 = {
      $push: {
         object1: 10,
         object2: "string",
         "object3.1": 20,
         "object4.1": { $date: "2016-05-16" }
      }
   };
   testPara.testCL.update( updateCondition1 );

   //check result
   var expRecs1 = [{
      object0: [10, -30, 20],
      object1: [10],
      object2: ["string"],
      object3: { 1: [20] },
      object4: { 1: [{ $date: "2016-05-16" }] }
   }];
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );

   //insert data
   var doc2 = [{ object5: [10, -30, 20] }];
   testPara.testCL.insert( doc2 );

   //update use push object does not exist,with matches
   var updateCondition2 = {
      $push: {
         object1: 10,
         object2: "string",
         "object3.2": 20,
         "object4.1": { $date: "2016-05-16" }
      }
   };
   var findCondition2 = { object1: { $exists: 1 } };
   testPara.testCL.update( updateCondition2, findCondition2 );

   //check result
   var expRecs2 = [{
      object0: [10, -30, 20],
      object1: [10, 10],
      object2: ["string", "string"],
      object3: { 1: [20], 2: [20] },
      object4: { 1: [{ $date: "2016-05-16" }, { $date: "2016-05-16" }] }
   },
   { object5: [10, -30, 20] }];
   checkResult( testPara.testCL, null, null, expRecs2, { _id: 1 } );
}

