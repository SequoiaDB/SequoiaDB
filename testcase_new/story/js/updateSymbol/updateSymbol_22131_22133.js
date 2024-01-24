/************************************
*@Description: seqDB-22131:使用inc更新对象为数值，$field指定字段值为非数值
               seqDB-22133:使用inc更新对象为非数值类型 
*@author:      wuyan
*@createdate:  2020.5.18
**************************************/

testConf.clOpt = { ShardingKey: { no: 1 } };
testConf.clName = COMMCLNAME + "_update_field_22131";
main( test );

function test ( testPara )
{
   var dbcl = testPara.testCL;
   var docs = [{ no: 0, a: -2, testb: "test0", num: 0 },
   { no: 1, a: { $numberLong: "9223372036854775000" }, testb: "test1", num: 123 },
   { no: 2, a: { num1: 20, num2: 0 }, testb: "test2", num: 4 },
   { no: 3, a: ["test0", 10.23, 0], testb: "test3", num: -234 },
   { no: 4, a: "string", testb: "test4", num: 666 },
   { no: 5, a: null, testb: 1, num: 777 }];
   dbcl.insert( docs );

   //$field指定字段值为非数值，更新字段为数值
   dbcl.update( { $inc: { a: { $field: 'testb' } } }, { no: { $lt: 2 } } );
   var cursor = dbcl.find( { "no": 0 } );
   commCompareResults( cursor, [docs[0]] );
   cursor = dbcl.find( { "no": 1 } );
   commCompareResults( cursor, [docs[1]] );

   //$field指定字段值为非数值，更新字段为对象，数组
   dbcl.update( { $inc: { 'a.num1': { $field: 'testb' } } }, { no: { $et: 2 } } );
   cursor = dbcl.find( { "no": 2 } );
   commCompareResults( cursor, [docs[2]] );

   dbcl.update( { $inc: { 'a.1': { $field: 'testb' } } }, { no: { $et: 3 } } );
   cursor = dbcl.find( { "no": 3 } );
   commCompareResults( cursor, [docs[3]] );

   //$field指定字段值为非数值，更新字段为字符串
   dbcl.update( { $inc: { a: { $field: 'testb' } } }, { no: { $et: 4 } } );
   cursor = dbcl.find( { "no": 4 } );
   commCompareResults( cursor, [docs[4]] );

   //$field指定字段值为数值，更新字段为null
   dbcl.update( { $inc: { a: { $field: 'testb' } } }, { no: { $et: 5 } } );
   cursor = dbcl.find( { "no": 5 } );
   commCompareResults( cursor, [docs[5]] );
}