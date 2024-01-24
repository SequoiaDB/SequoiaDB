/************************************
*@Description: seqDB-22149 : 使用pop指定field字段更新对象  
*@author:      wuyan
*@createdate:  2020.5.28
**************************************/

testConf.clName = COMMCLNAME + "_update_field_22149";
main( test );

function test ( testPara )
{
   var docs = [{ no: 0, a: -21470, b: -21470, testc: "test0" },
   { no: 1, a: 1, b: [-23.56, { $numberLong: "9223372036854775800" }, 124], testc: "test1" },
   { no: 2, a: 2, b: { no1: 23, no2: { test1: -12, test2: [0, 1, 0.25] } }, testc: "test2" },
   { no: 3, a: -3, b: [12.3, 555, 0, 2, { a: { no: 200034 } }], testc: "test3" }];

   testPara.testCL.insert( docs );

   //test a:更新对象非数组
   var updateCondition1 = { $pop: { b: { $field: "a" } } };
   var findCondition1 = { no: 0 };
   testPara.testCL.update( updateCondition1, findCondition1 );
   checkResult( testPara.testCL, null, null, docs, { _id: 1 } );

   //test b:更新对象为数组
   var updateCondition2 = { $pop: { b: { $field: "a" } } };
   var findCondition2 = { no: 1 };
   testPara.testCL.update( updateCondition2, findCondition2 );
   var expRecs2 = [{ no: 0, a: -21470, b: -21470, testc: "test0" },
   { no: 1, a: 1, b: [-23.56, { $numberLong: "9223372036854775800" }], testc: "test1" },
   { no: 2, a: 2, b: { no1: 23, no2: { test1: -12, test2: [0, 1, 0.25] } }, testc: "test2" },
   { no: 3, a: -3, b: [12.3, 555, 0, 2, { a: { no: 200034 } }], testc: "test3" }];
   checkResult( testPara.testCL, null, null, expRecs2, { _id: 1 } );

   var updateCondition3 = { $pop: { 'b.no2.test2': { $field: "a" } } };
   var findCondition3 = { no: 2 };
   testPara.testCL.update( updateCondition3, findCondition3 );
   var expRecs3 = [{ no: 0, a: -21470, b: -21470, testc: "test0" },
   { no: 1, a: 1, b: [-23.56, { $numberLong: "9223372036854775800" }], testc: "test1" },
   { no: 2, a: 2, b: { no1: 23, no2: { test1: -12, test2: [0] } }, testc: "test2" },
   { no: 3, a: -3, b: [12.3, 555, 0, 2, { a: { no: 200034 } }], testc: "test3" }];
   checkResult( testPara.testCL, null, null, expRecs3, { _id: 1 } );

   var updateCondition4 = { $pop: { 'b.no2.test2': { $field: "a" } } };
   var findCondition4 = { no: 3 };
   testPara.testCL.update( updateCondition4, findCondition4 );
   var expRecs4 = [{ no: 0, a: -21470, b: -21470, testc: "test0" },
   { no: 1, a: 1, b: [-23.56, { $numberLong: "9223372036854775800" }], testc: "test1" },
   { no: 2, a: 2, b: { no1: 23, no2: { test1: -12, test2: [0] } }, testc: "test2" },
   { no: 3, a: -3, b: [12.3, 555, 0, 2, { a: { no: 200034 } }], testc: "test3" }];
   checkResult( testPara.testCL, null, null, expRecs4, { _id: 1 } );

}

