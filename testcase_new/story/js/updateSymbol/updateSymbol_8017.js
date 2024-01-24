/************************************
*@Description: seqDB-8017:update同时使用多个更新符进行更新
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/
testConf.clName = COMMCLNAME + "_update_8017";
main( test );

function test ( testPara )
{
   //insert data
   var doc1 = [{ age: 1 },
   { arr: [1, "string", false, [40, null, { $date: "2016-05-20" }, 30], 10, 7, 10, 20] },
   { name: { firstName: "han", lastName: "meimei" } }];
   testPara.testCL.insert( doc1 );

   //update object use more than 1 update operator at the same time 
   var updateCondition1 = { $inc: { age: 100 }, $pull: { arr: 10 }, $set: { "name.firstName": "li" } };
   testPara.testCL.update( updateCondition1 );

   //check result
   var expRecs1 = [{ age: 101, name: { firstName: "li" } },
   { age: 100, arr: [1, "string", false, [40, null, { $date: "2016-05-20" }, 30], 7, 20], name: { firstName: "li" } },
   { age: 100, name: { firstName: "li", lastName: "meimei" } }];
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );
}
