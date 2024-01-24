/************************************
*@Description:  seqDB-8010:update使用push_all更新符更新已存在的对象
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/
testConf.clName = COMMCLNAME + "_push_all8010";
main( test );

function test ( testPara )
{
   //insert data
   var doc1 = [{ object1: [10, -30, { $numberLong: "20" }] },
   { object2: 12 },
   { object3: [10, -30, { $decimal: "50" }] },
   { object4: [200, ["string", -299, 400], 400] },
   { object6: [200, [305, -299, false, 1, 50, 1000], 400] },
   { object7: [200, [305, -299, [400, { $date: "2016-05-16" }, 50], 1000], 400] },
   { object8: [200, [305, -299, 400, 1, null, 1000], 400] }];
   testPara.testCL.insert( doc1 );

   //update use push_all exist object,no matches
   var updateCondition1 = {
      $push_all: {
         object1: [10, { $numberLong: "20" }],
         object2: [13, 15, 19],
         object3: [50, 10, 100, 105],
         object4: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
         "object6.1": [false, -299, 1000],
         "object7.1.2": [{ $date: "2016-05-16" }, 10],
         "object8.1.2": [20, 90, 10]
      }
   };
   testPara.testCL.update( updateCondition1 );

   //check result
   var expRecs1 = [{
      object1: [10, -30, 20, 10, 20],
      object2: [13, 15, 19],
      object3: [50, 10, 100, 105],
      object4: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
      object6: { 1: [false, -299, 1000] },
      object7: { 1: { 2: [{ $date: "2016-05-16" }, 10] } },
      object8: { 1: { 2: [20, 90, 10] } }
   },
   {
      object1: [10, 20],
      object2: 12,
      object3: [50, 10, 100, 105],
      object4: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
      object6: { 1: [false, -299, 1000] },
      object7: { 1: { 2: [{ $date: "2016-05-16" }, 10] } },
      object8: { 1: { 2: [20, 90, 10] } }
   },
   {
      object1: [10, 20],
      object2: [13, 15, 19],
      object3: [10, -30, { $decimal: "50" }, 50, 10, 100, 105],
      object4: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
      object6: { 1: [false, -299, 1000] },
      object7: { 1: { 2: [{ $date: "2016-05-16" }, 10] } },
      object8: { 1: { 2: [20, 90, 10] } }
   },
   {
      object1: [10, 20],
      object2: [13, 15, 19],
      object3: [50, 10, 100, 105],
      object4: [200, ["string", -299, 400], 400, { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
      object6: { 1: [false, -299, 1000] },
      object7: { 1: { 2: [{ $date: "2016-05-16" }, 10] } },
      object8: { 1: { 2: [20, 90, 10] } }
   },
   {
      object1: [10, 20],
      object2: [13, 15, 19],
      object3: [50, 10, 100, 105],
      object4: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
      object6: [200, [305, -299, false, 1, 50, 1000, false, -299, 1000], 400],
      object7: { 1: { 2: [{ $date: "2016-05-16" }, 10] } },
      object8: { 1: { 2: [20, 90, 10] } }
   },
   {
      object1: [10, 20],
      object2: [13, 15, 19],
      object3: [50, 10, 100, 105],
      object4: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
      object6: { 1: [false, -299, 1000] },
      object7: [200, [305, -299, [400, { $date: "2016-05-16" }, 50, { $date: "2016-05-16" }, 10], 1000], 400],
      object8: { 1: { 2: [20, 90, 10] } }
   },
   {
      object1: [10, 20],
      object2: [13, 15, 19],
      object3: [50, 10, 100, 105],
      object4: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
      object6: { 1: [false, -299, 1000] },
      object7: { 1: { 2: [{ $date: "2016-05-16" }, 10] } },
      object8: [200, [305, -299, 400, 1, null, 1000], 400]
   }];
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );

   //insert data
   var doc1 = [{ object5: [10, -30, { $numberLong: "20" }] }];
   testPara.testCL.insert( doc1 );

   //update use push exist object,with matches
   var updateCondition2 = {
      $push_all: {
         object1: [10, { $numberLong: "20" }],
         object2: [13, 15, 19],
         object3: [50, 10, 100, 105],
         object4: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
         "object6.1": [false, -299, 1000],
         "object7.1.2": [{ $date: "2016-05-16" }, 10],
         "object8.1.2": [20, 90, 10]
      }
   };
   var findCondition2 = { object5: { $exists: 1 } };
   testPara.testCL.update( updateCondition2, findCondition2 );

   //check result
   var expRecs2 = [{
      object1: [10, -30, 20, 10, 20],
      object2: [13, 15, 19],
      object3: [50, 10, 100, 105],
      object4: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
      object6: { 1: [false, -299, 1000] },
      object7: { 1: { 2: [{ $date: "2016-05-16" }, 10] } },
      object8: { 1: { 2: [20, 90, 10] } }
   },
   {
      object1: [10, 20],
      object2: 12,
      object3: [50, 10, 100, 105],
      object4: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
      object6: { 1: [false, -299, 1000] },
      object7: { 1: { 2: [{ $date: "2016-05-16" }, 10] } },
      object8: { 1: { 2: [20, 90, 10] } }
   },
   {
      object1: [10, 20],
      object2: [13, 15, 19],
      object3: [10, -30, { $decimal: "50" }, 50, 10, 100, 105],
      object4: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
      object6: { 1: [false, -299, 1000] },
      object7: { 1: { 2: [{ $date: "2016-05-16" }, 10] } },
      object8: { 1: { 2: [20, 90, 10] } }
   },
   {
      object1: [10, 20],
      object2: [13, 15, 19],
      object3: [50, 10, 100, 105],
      object4: [200, ["string", -299, 400], 400, { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
      object6: { 1: [false, -299, 1000] },
      object7: { 1: { 2: [{ $date: "2016-05-16" }, 10] } },
      object8: { 1: { 2: [20, 90, 10] } }
   },
   {
      object1: [10, 20],
      object2: [13, 15, 19],
      object3: [50, 10, 100, 105],
      object4: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
      object6: [200, [305, -299, false, 1, 50, 1000, false, -299, 1000], 400],
      object7: { 1: { 2: [{ $date: "2016-05-16" }, 10] } },
      object8: { 1: { 2: [20, 90, 10] } }
   },
   {
      object1: [10, 20],
      object2: [13, 15, 19],
      object3: [50, 10, 100, 105],
      object4: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
      object6: { 1: [false, -299, 1000] },
      object7: [200, [305, -299, [400, { $date: "2016-05-16" }, 50, { $date: "2016-05-16" }, 10], 1000], 400],
      object8: { 1: { 2: [20, 90, 10] } }
   },
   {
      object1: [10, 20],
      object2: [13, 15, 19],
      object3: [50, 10, 100, 105],
      object4: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
      object6: { 1: [false, -299, 1000] },
      object7: { 1: { 2: [{ $date: "2016-05-16" }, 10] } },
      object8: [200, [305, -299, 400, 1, null, 1000], 400]
   },
   {
      object5: [10, -30, 20],
      object1: [10, 20],
      object2: [13, 15, 19],
      object3: [50, 10, 100, 105],
      object4: [{ $binary: "aGVsbG8gd29ybGQ=", $type: "1" }],
      object6: { 1: [false, -299, 1000] },
      object7: { 1: { 2: [{ $date: "2016-05-16" }, 10] } },
      object8: { 1: { 2: [20, 90, 10] } }
   }];
   checkResult( testPara.testCL, null, null, expRecs2, { _id: 1 } );
}

