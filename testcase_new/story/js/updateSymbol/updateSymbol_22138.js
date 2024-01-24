/************************************
*@Description: seqDB-22138 :: 使用inc更新多个对象 
*@author:      wuyan
*@createdate:  2020.5.18
**************************************/

testConf.clName = COMMCLNAME + "_update_field_22138";
main( test );

function test ( testPara )
{
   var doc1 = [{ no: 0, a: 0, b: -21470, testc: "test0", num: 720368547, field: -345111 },
   { no: 1, a: 1, b: { $numberLong: "9223372036854775800" }, testc: "test1", num: { $decimal: "42345.02" }, field: 1.3e+10 },
   {
      no: 2, a: 20, b: ["test0", 10.23, [0, { $numberLong: "-92233720368547700" }]],
      testc: "test2", num: { no1: { $decimal: "42345.02" }, no2: 2 }, field: -23.45
   }];

   testPara.testCL.insert( doc1 );

   var updateCondition1 = { $inc: { a: { $field: 'field' }, no: { $field: 'b' }, num: { $field: 'no' } } };
   var findCondition1 = { testc: "test0" };
   testPara.testCL.update( updateCondition1, findCondition1 );
   var expRecs1 = [{ no: -21470, a: -345111, b: -21470, testc: "test0", num: 720368547, field: -345111 },
   { no: 1, a: 1, b: { $numberLong: "9223372036854775800" }, testc: "test1", num: { $decimal: "42345.02" }, field: 1.3e+10 },
   {
      no: 2, a: 20, b: ["test0", 10.23, [0, { $numberLong: "-92233720368547700" }]], testc: "test2",
      num: { no1: { $decimal: "42345.02" }, no2: 2 }, field: -23.45
   }];
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );

   var updateCondition2 = { $inc: { a: { $field: 'b.2.1' }, 'b.1': { $field: 'num.no2' }, no: { $field: 'num.no1' }, 'num.no2': { $field: 'field' } } };
   var findCondition2 = { testc: "test2" };
   testPara.testCL.update( updateCondition2, findCondition2 );
   var expRecs2 = [{ no: -21470, a: -345111, b: -21470, testc: "test0", num: 720368547, field: -345111 },
   { no: 1, a: 1, b: { $numberLong: "9223372036854775800" }, testc: "test1", num: { $decimal: "42345.02" }, field: 1.3e+10 },
   {
      no: { "$decimal": "42347.02" }, a: { $numberLong: "-92233720368547680" }, b: ["test0", 12.23, [0, { $numberLong: "-92233720368547700" }]], testc: "test2",
      num: { no1: { $decimal: "42345.02" }, no2: -21.45 }, field: -23.45
   }];
   checkResult( testPara.testCL, null, null, expRecs2, { _id: 1 } );

}

