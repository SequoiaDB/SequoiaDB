/************************************
*@Description: seqDB-22141 :使用inc更新后数值溢出  
*@author:      wuyan
*@createdate:  2020.5.19
**************************************/
testConf.clName = COMMCLNAME + "_update_field_22141";
main( test );

function test ( testPara )
{
   var docs = [{ no: 0, a: 12.34, testa: { $numberLong: "9000000000000000000" }, fieldb: { $numberLong: "223372036854775808" }, testc: "test0" },
   { no: 1, a: -1, testa: { $numberLong: "-1" }, fieldb: { $numberLong: "-9223372036854775808" }, testc: "test1" },
   { no: 2, a: 20, testa: 1, fieldb: { $numberLong: "9223372036854775807" }, testc: "test2" },
   { no: 3, a: 20, testa: -2147483648, fieldb: { $numberLong: "9203372036854775807" }, testc: "test2" }];
   testPara.testCL.insert( docs );

   var updateCondition = { $inc: { testa: { $field: 'fieldb' } } };
   testPara.testCL.update( updateCondition );
   var expRecs = [{ no: 0, a: 12.34, testa: { "$decimal": "9223372036854775808" }, fieldb: { $numberLong: "223372036854775808" }, testc: "test0" },
   { no: 1, a: -1, testa: { "$decimal": "-9223372036854775809" }, fieldb: { $numberLong: "-9223372036854775808" }, testc: "test1" },
   { no: 2, a: 20, testa: { "$decimal": "9223372036854775808" }, fieldb: { $numberLong: "9223372036854775807" }, testc: "test2" },
   { no: 3, a: 20, testa: { "$numberLong": "9203372034707292159" }, fieldb: { $numberLong: "9203372036854775807" }, testc: "test2" }];
   checkResult( testPara.testCL, null, null, expRecs, { _id: 1 } );
}


