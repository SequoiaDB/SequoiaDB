/************************************
*@Description: seqDB-22136 :: 使用inc更新对象，指定Min和Max
*@author:      wuyan
*@createdate:  2020.5.18
**************************************/

testConf.clName = COMMCLNAME + "_update_field_22136";
main( test );

function test ( testPara )
{
   var doc1 = [{ no: 0, a: 1, fieldb: -2147, testc: "test0" },
   { no: 1, a: 1, fieldb: { $numberLong: "9223372036854775800" }, testc: "test1" },
   { no: 2, a: -20, fieldb: 100.707, testc: "test2" },
   { no: 3, a: { $decimal: "-2" }, fieldb: { $decimal: "-20" }, testc: "test3" },
   { no: 4, a: -234, a1: 20, fieldb: { num1: 20, num2: 0 }, fieldb1: { num: { a: 12.01 } }, testc: "test4" },
   { no: 5, a: -1203333, a1: 1203333, fieldb: ["test0", 10.23, [0, -123]], testc: "test5" }];
   testPara.testCL.insert( doc1 );

   //test a:更新后值在Min和Max之间
   var updateCondition1 = { $inc: { a: { Value: { $field: 'fieldb' }, Min: -10000, Max: { $numberLong: "9223372036854775802" } } } };
   var findCondition1 = { no: { $lte: 3 } };
   testPara.testCL.update( updateCondition1, findCondition1 );
   var expRecs1 = [{ no: 0, a: -2146, fieldb: -2147, testc: "test0" },
   { no: 1, a: { $numberLong: "9223372036854775801" }, fieldb: { $numberLong: "9223372036854775800" }, testc: "test1" },
   { no: 2, a: 80.707, fieldb: 100.707, testc: "test2" },
   { no: 3, a: { $decimal: "-22" }, fieldb: { $decimal: "-20" }, testc: "test3" },
   { no: 4, a: -234, a1: 20, fieldb: { num1: 20, num2: 0 }, fieldb1: { num: { a: 12.01 } }, testc: "test4" },
   { no: 5, a: -1203333, a1: 1203333, fieldb: ["test0", 10.23, [0, -123]], testc: "test5" }];
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );

   //test b:更新后值小于Min
   var updateCondition2 = { $inc: { a: { Value: { $field: 'fieldb.num1' }, Min: 0, Max: 100 } } };
   var findCondition2 = { no: { $et: 4 } };
   updateError( updateCondition2, findCondition2 );
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );

   //test c:更新后运算值大于Max
   var updateCondition3 = { $inc: { a: { Value: { $field: 'fieldb' }, Min: 0, Max: 100 } } };
   var findCondition3 = { no: { $et: 2 } };
   updateError( updateCondition3, findCondition3 );
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );


   //test d:更新后运算值等于Min、Max
   var updateCondition4 = { $inc: { a: { Value: { $field: 'a1' }, Min: -214, Max: 0 } } };
   var findCondition4 = { no: { $gte: 4 } };
   testPara.testCL.update( updateCondition4, findCondition4 );
   var expRecs4 = [{ no: 0, a: -2146, fieldb: -2147, testc: "test0" },
   { no: 1, a: { $numberLong: "9223372036854775801" }, fieldb: { $numberLong: "9223372036854775800" }, testc: "test1" },
   { no: 2, a: 80.707, fieldb: 100.707, testc: "test2" },
   { no: 3, a: { $decimal: "-22" }, fieldb: { $decimal: "-20" }, testc: "test3" },
   { no: 4, a: -214, a1: 20, fieldb: { num1: 20, num2: 0 }, fieldb1: { num: { a: 12.01 } }, testc: "test4" },
   { no: 5, a: 0, a1: 1203333, fieldb: ["test0", 10.23, [0, -123]], testc: "test5" }];
   checkResult( testPara.testCL, null, null, expRecs4, { _id: 1 } );
}

function updateError ( updateCondition, findCondition )
{
   assert.tryThrow( SDB_VALUE_OVERFLOW, function()
   {
      testPara.testCL.update( updateCondition, findCondition );
   } );
}
