/************************************
*@Description: seqDB-22140 :开启严格数据控制，使用inc更新后数值溢出  
*@author:      wuyan
*@createdate:  2020.5.19
**************************************/
testConf.clName = COMMCLNAME + "_update_field_22140";
testConf.clOpt = { ShardingKey: { no: 1 }, StrictDataMode: true };
main( test );

function test ( testPara )
{
   var docs = [{ no: 0, a: 12.34, testa: { $numberLong: "9000000000000000000" }, fieldb: { $numberLong: "223372036854775808" }, testc: "test0" },
   { no: 1, a: -1, testa: -2147483648, fieldb: -10.67, testc: "test1" },
   { no: 2, a: 20, testa: 2147483658, fieldb: 1.7e+308, testc: "test2" }];
   testPara.testCL.insert( docs );

   var updateCondition = { $inc: { testa: { $field: 'fieldb' } } };
   var findCondition = { no: { $et: 0 } };
   updateError( updateCondition, findCondition );
   checkResult( testPara.testCL, null, null, docs, { _id: 1 } );

   var updateCondition1 = { $inc: { testa: { $field: 'fieldb' } } };
   var findCondition1 = { no: { $gt: 0 } };
   testPara.testCL.update( updateCondition1, findCondition1 );
   var expRecs1 = [{ no: 0, a: 12.34, testa: { $numberLong: "9000000000000000000" }, fieldb: { $numberLong: "223372036854775808" }, testc: "test0" },
   { no: 1, a: -1, testa: -2147483658.67, fieldb: -10.67, testc: "test1" },
   { no: 2, a: 20, testa: 1.7e+308, fieldb: 1.7e+308, testc: "test2" }];
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );
}

function updateError ( updateCondition, findCondition )
{
   assert.tryThrow( SDB_VALUE_OVERFLOW, function()
   {
      testPara.testCL.update( updateCondition, findCondition );
   } )
}