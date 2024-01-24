/************************************
*@Description: seqDB-8012:update使用replace更新符更新不存在的对象
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/
testConf.clName = COMMCLNAME + "_replace8012";
main( test );

function test ( testPara )
{
   //insert data   
   var doc1 = [{ object: 123 },
   { "object5.0": { $oid: "573920accc332f037c000013" } }];
   testPara.testCL.insert( doc1 );

   //update use replace object does not exist,no matches
   var updateCondition1 = {
      $replace: {
         object1: 10,
         object2: { $date: "2016-05-16" },
         object3: [10, 5, 7],
         object4: { firstName: "han", lastName: "meimei" }
      }
   };
   testPara.testCL.update( updateCondition1 );

   //check result
   var expRecs1 = [{
      object1: 10,
      object2: { $date: "2016-05-16" },
      object3: [10, 5, 7],
      object4: { firstName: "han", lastName: "meimei" }
   },
   {
      object1: 10,
      object2: { $date: "2016-05-16" },
      object3: [10, 5, 7],
      object4: { firstName: "han", lastName: "meimei" }
   }];
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );

   //insert data
   var doc2 = [{ object: [10, -30, 20] }];
   testPara.testCL.insert( doc2 );

   //update use replace object does not exist,with matches
   var updateCondition2 = {
      $replace: {
         object1: 10,
         object2: { $date: "2016-05-16" },
         object3: [10, 5, 7],
         object4: { firstName: "han", lastName: "meimei" }
      }
   };
   var findCondition2 = { object: { $exists: 1 } };
   testPara.testCL.update( updateCondition2, findCondition2 );

   //check result
   var expRecs2 = [{
      object1: 10,
      object2: { $date: "2016-05-16" },
      object3: [10, 5, 7],
      object4: { firstName: "han", lastName: "meimei" }
   },
   {
      object1: 10,
      object2: { $date: "2016-05-16" },
      object3: [10, 5, 7],
      object4: { firstName: "han", lastName: "meimei" }
   },
   {
      object1: 10,
      object2: { $date: "2016-05-16" },
      object3: [10, 5, 7],
      object4: { firstName: "han", lastName: "meimei" }
   }];
   checkResult( testPara.testCL, null, null, expRecs2, { _id: 1 } );

   //update object's name can not include $ or .,the expect result is failed;
   var updateCondition3 = { $replace: { "object1.0": 10 } };
   invalidDataUpdateCheckResult( testPara.testCL, updateCondition3, -6 );

   var updateCondition4 = { $replace: { $object1: 10 } };
   invalidDataUpdateCheckResult( testPara.testCL, updateCondition4, -6 );
}
