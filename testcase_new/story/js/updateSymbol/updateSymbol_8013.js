/************************************
*@Description:  seqDB-8013:update使用replace更新符更新已存在的任意类型对象
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/
testConf.clName = COMMCLNAME + "_replace8013";
main( test );

function test ( testPara )
{
   //insert data   
   var doc1 = [{ object1: 123 },
   { "object2.0": { $oid: "573920accc332f037c000013" } }];
   testPara.testCL.insert( doc1 );

   //update use replace exist object,no matches
   var updateCondition1 = {
      $replace: {
         object1: 10,
         object2: { $date: "2016-05-16" }
      }
   };
   testPara.testCL.update( updateCondition1 );

   //check result
   var expRecs1 = [{
      object1: 10,
      object2: { $date: "2016-05-16" }
   },
   {
      object1: 10,
      object2: { $date: "2016-05-16" }
   }];
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );

   //insert data
   var doc2 = [{ object: [10, -30, 20] }];
   testPara.testCL.insert( doc2 );

   //update use replace exist object,with matches
   var updateCondition2 = {
      $replace: {
         object3: [10, 5, 7],
         object4: { firstName: "han", lastName: "meimei" }
      }
   };
   var findCondition2 = { object: { $exists: 1 } };
   testPara.testCL.update( updateCondition2, findCondition2 );

   //check result
   var expRecs2 = [{
      object1: 10,
      object2: { $date: "2016-05-16" }
   },
   {
      object1: 10,
      object2: { $date: "2016-05-16" }
   },
   {
      object3: [10, 5, 7],
      object4: { firstName: "han", lastName: "meimei" }
   }];
   checkResult( testPara.testCL, null, null, expRecs2, { _id: 1 } );

}
