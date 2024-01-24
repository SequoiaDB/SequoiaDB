/************************************
*@Description: seqDB-8016:使用任意一个更新符update索引
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/

testConf.clName = COMMCLNAME + "_update_8016";
main( test );

function test ( testPara )
{
   //create index
   commCreateIndex( testPara.testCL, "ageIndex", { age: 1 } );
   commCreateIndex( testPara.testCL, "arr", { arr: -1 } );
   commCreateIndex( testPara.testCL, "name", { name: 1 } );

   //insert data
   var doc1 = [{ age: 1 },
   { arr: [1, "string", false, [40, null, { $date: "2016-05-20" }, 30], 10, 7, 10, 20] },
   { name: { firstName: "han", lastName: "meimei" } }];
   testPara.testCL.insert( doc1 );

   //update common object as index 
   var updateCondition1 = { $set: { age: 100 } };
   var findCondition1 = { age: { $exists: 1 } };
   testPara.testCL.update( updateCondition1, findCondition1 );

   //check result
   var expRecs1 = [{ age: 100 },
   { arr: [1, "string", false, [40, null, { $date: "2016-05-20" }, 30], 10, 7, 10, 20] },
   { name: { firstName: "han", lastName: "meimei" } }];
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );

   //update arr object as index 
   var updateCondition2 = { $pull: { arr: 10 } };
   var findCondition2 = { arr: { $exists: 1 } };
   testPara.testCL.update( updateCondition2, findCondition2 );

   //check result
   var expRecs2 = [{ age: 100 },
   { arr: [1, "string", false, [40, null, { $date: "2016-05-20" }, 30], 7, 20] },
   { name: { firstName: "han", lastName: "meimei" } }];
   checkResult( testPara.testCL, null, null, expRecs2, { _id: 1 } );

   //update nested arr's element 
   var updateCondition3 = { $pull: { "arr.3": null } };
   var findCondition3 = { arr: { $exists: 1 } };
   testPara.testCL.update( updateCondition3, findCondition3 );

   //check result
   var expRecs3 = [{ age: 100 },
   { arr: [1, "string", false, [40, { $date: "2016-05-20" }, 30], 7, 20] },
   { name: { firstName: "han", lastName: "meimei" } }];
   checkResult( testPara.testCL, null, null, expRecs3, { _id: 1 } );

   //update nested object 
   var updateCondition4 = { $set: { "name.firstName": "li" } };
   var findCondition4 = { name: { $exists: 1 } };
   testPara.testCL.update( updateCondition4, findCondition4 );

   //check result
   var expRecs4 = [{ age: 100 },
   { arr: [1, "string", false, [40, { $date: "2016-05-20" }, 30], 7, 20] },
   { name: { firstName: "li", lastName: "meimei" } }];
   checkResult( testPara.testCL, null, null, expRecs4, { _id: 1 } );
}
