/************************************
*@Description: seqDB-22132:使用inc更新对象为数值，$field指定字段值和更新字段值类型不一致 
               seqDB-22139:inc更新对象为分区键
*@author:      wuyan
*@createdate:  2020.5.19
**************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_update_field_22132";
testConf.clOpt = { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range", Compressed: true };
main( test );

function test ( testPara )
{
   var subclName1 = COMMCLNAME + "_update_field_22132a";
   var subclName2 = COMMCLNAME + "_update_field_22132b";
   commDropCL( db, COMMCSNAME, subclName1, true, true );
   commDropCL( db, COMMCSNAME, subclName2, true, true );
   commCreateCL( db, COMMCSNAME, subclName1, { ShardingKey: { no: 1 }, ShardingType: "hash" } );
   commCreateCL( db, COMMCSNAME, subclName2, { ShardingKey: { no: 1 }, ShardingType: "range" } );
   testPara.testCL.attachCL( COMMCSNAME + "." + subclName1, { "LowBound": { a: -200 }, "UpBound": { a: 100 } } );
   testPara.testCL.attachCL( COMMCSNAME + "." + subclName2, { "LowBound": { a: 100 }, "UpBound": { a: MaxKey() } } );

   var docs = [{ no: 0, a: 12.34, testa: 12.34, fieldb: -21470, testc: "test0" },
   { no: 1, a: -1, testa: -1, fieldb: { $numberLong: "9223372036854775800" }, testc: "test1" },
   { no: 2, a: 20, testa: 20, fieldb: 1.6e+300, testc: "test2" },
   { no: 3, a: { $numberLong: "30000000000" }, testa: { $numberLong: "30000000000" }, fieldb: { $decimal: "20" }, testc: "test3" },
   { no: 4, a: 2147483640, testa: 2147483640, fieldb: { num1: { a: 12.01 }, num2: 0 }, testc: "test4" },
   { no: 5, a: -10.23, testa: -1000000.23, fieldb: ["test0", 0, [1000000.23, -123]], testc: "test5" }];
   testPara.testCL.insert( docs );

   //$field指定字段值为数值类型
   var updateCondition1 = { $inc: { testa: { $field: 'fieldb' } } };
   var findCondition1 = { no: { $lte: 3 } };
   testPara.testCL.update( updateCondition1, findCondition1 );
   var expRecs1 = [{ no: 0, a: 12.34, testa: -21457.66, fieldb: -21470, testc: "test0" },
   { no: 1, a: -1, testa: { $numberLong: "9223372036854775799" }, fieldb: { $numberLong: "9223372036854775800" }, testc: "test1" },
   { no: 2, a: 20, testa: 1.6e+300, fieldb: 1.6e+300, testc: "test2" },
   { no: 3, a: 30000000000, testa: { "$decimal": "30000000020" }, fieldb: { $decimal: "20" }, testc: "test3" },
   { no: 4, a: 2147483640, testa: 2147483640, fieldb: { num1: { a: 12.01 }, num2: 0 }, testc: "test4" },
   { no: 5, a: -10.23, testa: -1000000.23, fieldb: ["test0", 0, [1000000.23, -123]], testc: "test5" }];
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );

   //$field指定字段值为对象类型
   var updateCondition2 = { $inc: { testa: { $field: 'fieldb.num1.a' } } };
   var findCondition2 = { no: { $et: 4 } };
   testPara.testCL.update( updateCondition2, findCondition2 );
   var expRecs2 = [{ no: 0, a: 12.34, testa: -21457.66, fieldb: -21470, testc: "test0" },
   { no: 1, a: -1, testa: { $numberLong: "9223372036854775799" }, fieldb: { $numberLong: "9223372036854775800" }, testc: "test1" },
   { no: 2, a: 20, testa: 1.6e+300, fieldb: 1.6e+300, testc: "test2" },
   { no: 3, a: 30000000000, testa: { "$decimal": "30000000020" }, fieldb: { $decimal: "20" }, testc: "test3" },
   { no: 4, a: 2147483640, testa: 2147483652.01, fieldb: { num1: { a: 12.01 }, num2: 0 }, testc: "test4" },
   { no: 5, a: -10.23, testa: -1000000.23, fieldb: ["test0", 0, [1000000.23, -123]], testc: "test5" }];
   checkResult( testPara.testCL, null, null, expRecs2, { _id: 1 } );

   //$field指定字段值为数组类型
   var updateCondition3 = { $inc: { testa: { $field: 'fieldb.2.0' } } };
   var findCondition3 = { no: { $et: 5 } };
   testPara.testCL.update( updateCondition3, findCondition3 );
   var expRecs3 = [{ no: 0, a: 12.34, testa: -21457.66, fieldb: -21470, testc: "test0" },
   { no: 1, a: -1, testa: { $numberLong: "9223372036854775799" }, fieldb: { $numberLong: "9223372036854775800" }, testc: "test1" },
   { no: 2, a: 20, testa: 1.6e+300, fieldb: 1.6e+300, testc: "test2" },
   { no: 3, a: 30000000000, testa: { "$decimal": "30000000020" }, fieldb: { $decimal: "20" }, testc: "test3" },
   { no: 4, a: 2147483640, testa: 2147483652.01, fieldb: { num1: { a: 12.01 }, num2: 0 }, testc: "test4" },
   { no: 5, a: -10.23, testa: 0, fieldb: ["test0", 0, [1000000.23, -123]], testc: "test5" }];
   checkResult( testPara.testCL, null, null, expRecs3, { _id: 1 } );

   //inc更新对象为分区键
   var updateCondition4 = { $inc: { a: { $field: 'testa' } } };
   testPara.testCL.update( updateCondition4 );

   //未更新记录
   checkResult( testPara.testCL, null, null, expRecs3, { _id: 1 } );
}

