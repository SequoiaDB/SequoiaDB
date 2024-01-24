/************************************
*@Description: seqDB-8015:使用任意一个更新符update分区键
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/
testConf.skipStandAlone = true;
testConf.clOpt = { ShardingKey: { "age": 1 }, ShardingType: "range" };
testConf.clName = COMMCLNAME + "_update_8015";
main( test );

function test ( testPara )
{
   //insert data
   var doc1 = [{ age: 1 }, { age: 2 }];
   testPara.testCL.insert( doc1 );

   //update common data
   var updateCondition1 = { $unset: { age: "" } };
   testPara.testCL.update( updateCondition1 );

   //check result
   var expRecs1 = [{ age: 1 }, { age: 2 }];
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );

   //insert data
   var doc2 = [{ age: [1, 2, 3] }];
   testPara.testCL.insert( doc2 );

   //update common data
   var updateCondition2 = { $addtoset: { age: [3, 4, 5, 6] } };
   testPara.testCL.update( updateCondition2 );

   //check result
   var expRecs1 = [{ age: 1 }, { age: 2 }, { age: [1, 2, 3] }];
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );
}
