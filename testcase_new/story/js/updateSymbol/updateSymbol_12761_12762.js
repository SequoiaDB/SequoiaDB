/************************************
*@Description: seqDB-12761:update使用pull_all_by更新非数组对象
               seqDB-12762:update使用pull_all_by更新空数组对象
*@author:      liuxiaoxuan
*@createdate:  2017.09.19
**************************************/
testConf.clName = COMMCLNAME + "_pull_all_by_12761";
main( test );

function test ( testPara )
{
   //insert data   
   var doc = [{ a1: 1 },
   { a2: 'aaa' },
   { a3: [] }];
   testPara.testCL.insert( doc );

   //pull_by
   var updateRule = { $pull_all_by: { a1: [1], a2: ['aaa'], a3: [[]] } };
   testPara.testCL.update( updateRule );

   //check result
   var expResult = [{ a1: 1 },
   { a2: 'aaa' },
   { a3: [] }];
   checkResult( testPara.testCL, null, null, expResult, { _id: 1 } );
}

