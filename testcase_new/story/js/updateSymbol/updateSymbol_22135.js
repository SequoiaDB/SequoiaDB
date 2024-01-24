/************************************
*@Description: seqDB-22135 :: 使用inc更新对象，指定Default字段 
*@author:      wuyan
*@createdate:  2020.5.18
**************************************/

testConf.clName = COMMCLNAME + "_update_field_22135";
main( test );

function test ( testPara )
{
   var docs = [{ no: 0, a: -21470, fieldb: -21470, testc: "test0" },
   { no: 1, a: 92233720, fieldb: { $numberLong: "9223372036854775800" }, testc: "test1" },
   { no: 2, a: 120.707, fieldb: 100.707, testc: "test2" },
   { no: 3, a: { $decimal: "22" }, fieldb: { $decimal: "20" }, testc: "test3" }];

   testPara.testCL.insert( docs );

   //test a:更新对象不存在，指定Default为null
   var updateCondition1 = { $inc: { 'testa': { Value: { $field: 'a' }, Default: null } } };
   testPara.testCL.update( updateCondition1 );
   checkResult( testPara.testCL, null, null, docs, { _id: 1 } );

   //test b:更新对象不存在，指定Default为数值类型
   var updateCondition2 = { $inc: { 'testb': { Value: { $field: 'a' }, Default: -2345.68 } } };
   var findCondition2 = { no: { $et: 1 } };
   testPara.testCL.update( updateCondition2, findCondition2 );
   var expRecs2 = [{ no: 0, a: -21470, fieldb: -21470, testc: "test0" },
   { no: 1, a: 92233720, fieldb: { $numberLong: "9223372036854775800" }, testc: "test1", testb: 92231374.32 },
   { no: 2, a: 120.707, fieldb: 100.707, testc: "test2" },
   { no: 3, a: { $decimal: "22" }, fieldb: { $decimal: "20" }, testc: "test3" }];
   checkResult( testPara.testCL, null, null, expRecs2, { _id: 1 } );

   var updateCondition3 = { $inc: { 'testb2': { Value: { $field: 'a' }, Default: { $decimal: "4563330" } } } };
   testPara.testCL.update( updateCondition3 );
   var expRecs3 = [{ no: 0, a: -21470, fieldb: -21470, testc: "test0", testb2: { $decimal: "4541860" } },
   { no: 1, a: 92233720, fieldb: { $numberLong: "9223372036854775800" }, testc: "test1", testb: 92231374.32, testb2: { $decimal: "96797050" } },
   { no: 2, a: 120.707, fieldb: 100.707, testc: "test2", testb2: { $decimal: "4563450.707" } },
   { no: 3, a: { $decimal: "22" }, fieldb: { $decimal: "20" }, testc: "test3", testb2: { $decimal: "4563352" } }];
   checkResult( testPara.testCL, null, null, expRecs3, { _id: 1 } );

   //test c:指定Default
   var updateCondition4 = { $inc: { 'a': { Value: { $field: 'fieldb' }, Default: 32 } } };
   var findCondition4 = { no: { $gt: 1 } };
   testPara.testCL.update( updateCondition4, findCondition4 );
   var expRecs4 = [{ no: 0, a: -21470, fieldb: -21470, testc: "test0", testb2: { $decimal: "4541860" } },
   { no: 1, a: 92233720, fieldb: { $numberLong: "9223372036854775800" }, testc: "test1", testb: 92231374.32, testb2: { $decimal: "96797050" } },
   { no: 2, a: 221.414, fieldb: 100.707, testc: "test2", testb2: { $decimal: "4563450.707" } },
   { no: 3, a: { $decimal: "42" }, fieldb: { $decimal: "20" }, testc: "test3", testb2: { $decimal: "4563352" } }];
   checkResult( testPara.testCL, null, null, expRecs4, { _id: 1 } );
}

