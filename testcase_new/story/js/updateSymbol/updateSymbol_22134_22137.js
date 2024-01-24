/************************************
*@Description: seqDB-22134 :: 使用inc更新不存在的对象 
               seqDB-22137 :: 使用inc更新对象，$field指定字段不存在
*@author:      wuyan
*@createdate:  2020.5.18
**************************************/

testConf.clName = COMMCLNAME + "_update_field_22134";
main( test );

function test ( testPara )
{
   var docs = [{ no: 0, a: -21470, fieldb: -21470, testc: "test0" },
   { no: 1, a: { $numberLong: "9223372036854775801" }, fieldb: { $numberLong: "9223372036854775800" }, testc: "test1" },
   { no: 2, a: 120.707, fieldb: 100.707, testc: "test2" },
   { no: 3, a: { $decimal: "22" }, fieldb: { $decimal: "20" }, testc: "test3" }];

   testPara.testCL.insert( docs );

   //testcase22137：$field指定字段不存在
   var updateCondition = { $inc: { 'no': { $field: 'testno' } } };
   testPara.testCL.update( updateCondition );
   checkResult( testPara.testCL, null, null, docs, { _id: 1 } );

   //testcase22134：不使用匹配符更新
   var updateCondition1 = { $inc: { 'testno': { $field: 'fieldb' } } };
   testPara.testCL.update( updateCondition1 );
   var expRecs1 = [{ no: 0, a: -21470, fieldb: -21470, testc: "test0", testno: -21470 },
   { no: 1, a: { $numberLong: "9223372036854775801" }, fieldb: { $numberLong: "9223372036854775800" }, testc: "test1", testno: { $numberLong: "9223372036854775800" } },
   { no: 2, a: 120.707, fieldb: 100.707, testc: "test2", testno: 100.707 },
   { no: 3, a: { $decimal: "22" }, fieldb: { $decimal: "20" }, testc: "test3", testno: { $decimal: "20" } }];
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );

   //testcase22134：使用匹配符更新
   var updateCondition2 = { $inc: { 'testno1': { $field: 'a' } } };
   var findCondition2 = { no: { $gt: 1 } };
   testPara.testCL.update( updateCondition2, findCondition2 );
   var expRecs2 = [{ no: 0, a: -21470, fieldb: -21470, testc: "test0", testno: -21470 },
   { no: 1, a: { $numberLong: "9223372036854775801" }, fieldb: { $numberLong: "9223372036854775800" }, testc: "test1", testno: { $numberLong: "9223372036854775800" } },
   { no: 2, a: 120.707, fieldb: 100.707, testc: "test2", testno: 100.707, testno1: 120.707 },
   { no: 3, a: { $decimal: "22" }, fieldb: { $decimal: "20" }, testc: "test3", testno: { $decimal: "20" }, testno1: { $decimal: "22" } }];
   checkResult( testPara.testCL, null, null, expRecs2, { _id: 1 } );
}

