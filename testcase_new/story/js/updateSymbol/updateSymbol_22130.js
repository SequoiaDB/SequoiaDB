/************************************
*@Description: seqDB-22130:使用inc更新对象为数值，$field指定字段值为数值
*@author:      wuyan
*@createdate:  2020.5.18
**************************************/

testConf.clName = COMMCLNAME + "_update_field_22130";
main( test );

function test ( testPara )
{
   var doc1 = [{ no: 0, a: 0, fieldb: -21470, testc: "test0" },
   { no: 1, a: 1, fieldb: { $numberLong: "9223372036854775800" }, testc: "test1" },
   { no: 2, a: 20, fieldb: 100.707, testc: "test2" },
   { no: 3, a: { $decimal: "2" }, fieldb: { $decimal: "20" }, testc: "test3" },
   { no: 4, a: -234, a1: 20, fieldb: { num1: 20, num2: 0 }, fieldb1: { num: { a: 12.01 } }, testc: "test4" },
   { no: 5, a: -10.23, a1: 1203333, fieldb: ["test0", 10.23, [0, -123]], testc: "test5" }];
   testPara.testCL.insert( doc1 );

   //$field指定字段值为数值类型
   var updateCondition1 = { $inc: { a: { $field: 'fieldb' } } };
   var findCondition1 = { no: { $lte: 3 } };
   testPara.testCL.update( updateCondition1, findCondition1 );
   var expRecs1 = [{ no: 0, a: -21470, fieldb: -21470, testc: "test0" },
   { no: 1, a: { $numberLong: "9223372036854775801" }, fieldb: { $numberLong: "9223372036854775800" }, testc: "test1" },
   { no: 2, a: 120.707, fieldb: 100.707, testc: "test2" },
   { no: 3, a: { $decimal: "22" }, fieldb: { $decimal: "20" }, testc: "test3" },
   { no: 4, a: -234, a1: 20, fieldb: { num1: 20, num2: 0 }, fieldb1: { num: { a: 12.01 } }, testc: "test4" },
   { no: 5, a: -10.23, a1: 1203333, fieldb: ["test0", 10.23, [0, -123]], testc: "test5" }];
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );

   //$field指定字段值为对象类型
   var updateCondition2 = { $inc: { a: { $field: 'fieldb.num1' }, a1: { $field: 'fieldb1.num.a' } } };
   var findCondition2 = { no: { $gt: 3 } };
   testPara.testCL.update( updateCondition2, findCondition2 );
   var expRecs2 = [{ no: 0, a: -21470, fieldb: -21470, testc: "test0" },
   { no: 1, a: { $numberLong: "9223372036854775801" }, fieldb: { $numberLong: "9223372036854775800" }, testc: "test1" },
   { no: 2, a: 120.707, fieldb: 100.707, testc: "test2" },
   { no: 3, a: { $decimal: "22" }, fieldb: { $decimal: "20" }, testc: "test3" },
   { no: 4, a: -214, a1: 32.01, fieldb: { num1: 20, num2: 0 }, fieldb1: { num: { a: 12.01 } }, testc: "test4" },
   { no: 5, a: -10.23, a1: 1203333, fieldb: ["test0", 10.23, [0, -123]], testc: "test5" }];
   checkResult( testPara.testCL, null, null, expRecs2, { _id: 1 } );

   //$field指定字段值为数组类型
   var updateCondition3 = { $inc: { a: { $field: 'fieldb.1' }, a1: { $field: 'fieldb.2.1' } } };
   var findCondition3 = { no: { $et: 5 } };
   testPara.testCL.update( updateCondition3, findCondition3 );
   var expRecs3 = [{ no: 0, a: -21470, fieldb: -21470, testc: "test0" },
   { no: 1, a: { $numberLong: "9223372036854775801" }, fieldb: { $numberLong: "9223372036854775800" }, testc: "test1" },
   { no: 2, a: 120.707, fieldb: 100.707, testc: "test2" },
   { no: 3, a: { $decimal: "22" }, fieldb: { $decimal: "20" }, testc: "test3" },
   { no: 4, a: -214, a1: 32.01, fieldb: { num1: 20, num2: 0 }, fieldb1: { num: { a: 12.01 } }, testc: "test4" },
   { no: 5, a: 0, a1: 1203210, fieldb: ["test0", 10.23, [0, -123]], testc: "test5" }];
   checkResult( testPara.testCL, null, null, expRecs3, { _id: 1 } );
}

