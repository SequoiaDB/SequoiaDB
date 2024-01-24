/************************************
*@Description: seqDB-7985:update使用inc更新符更新不存在的对象
               seqDB-7986:update使用inc更新符更新存在的对象
*@author:      zhaoyu
*@createdate:  2016.5.16
**************************************/

testConf.clName = COMMCLNAME + "_inc7985";
main( test );

function test ( testPara )
{
   //insert numberic data,array with 3 layer and common object  
   var doc1 = [{ a: -2147483640, c: -2147483640, e: { name: { firstName: "han", lastName: "meimei" } }, f: { name: { firstNumber: { $decimal: "1" }, lastNumber: { $numberLong: "2" } } } },
   { b: 2147483640, c: 2147483640, d: [1, 2, 3], f: { name: { firstNumber: { $decimal: "1" }, lastNumber: { $numberLong: "2" } } } },
   { a: { $numberLong: "-9223372036854775800" }, b: { $numberLong: "-9223372036854775800" }, d: [1, 2, 3], e: { name: { firstName: "han", lastName: "meimei" } } },
   { b: { $numberLong: "9223372036854775800" }, c: { $numberLong: "123" } },
   { a: -1.7E+308, c: -1.7E+308 },
   { a: 1.7E+308, b: 1.7E+308 },
   { b: -4.9E-324, c: -4.9E-324 },
   { a: 4.9E-324, c: 4.9E-324 }];
   testPara.testCL.insert( doc1 );

   //update use $inc,the operate object is exist or not exist
   var updateCondition1 = { $inc: { a: 1, b: { $numberLong: "-1" }, c: 1.56789, "d.0": { $decimal: "2" }, "e.name.firstName": 100, "f.name.firstNumber": 1000 } };
   testPara.testCL.update( updateCondition1 )

   //check result
   var expRecs1 = [{ a: -2147483639, b: -1, c: -2147483638.43211, d: { 0: { $decimal: "2" } }, e: { name: { firstName: "han", lastName: "meimei" } }, f: { name: { firstNumber: { $decimal: "1001" }, lastNumber: 2 } } },
   { a: 1, b: 2147483639, c: 2147483641.56789, d: [{ $decimal: "3" }, 2, 3], e: { name: { firstName: 100 } }, f: { name: { firstNumber: { $decimal: "1001" }, lastNumber: 2 } } },
   { a: { $numberLong: "-9223372036854775799" }, b: { $numberLong: "-9223372036854775801" }, c: 1.56789, d: [{ $decimal: "3" }, 2, 3], e: { name: { firstName: "han", lastName: "meimei" } }, f: { name: { firstNumber: 1000 } } },
   { a: 1, b: { $numberLong: "9223372036854775799" }, c: 124.56789, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } },
   { a: -1.7E+308, b: -1, c: -1.7E+308, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } },
   { a: 1.7E+308, b: 1.7E+308, c: 1.56789, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } },
   { a: 1, b: -1, c: 1.56789, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } },
   { a: 1, b: -1, c: 1.56789, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } }];
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );

   //insert data for updating data with matches symbol
   var doc2 = { g: 1 }
   testPara.testCL.insert( doc2 );

   //update when matches condition
   var updateCondition2 = { $inc: { g: 1, h: 1 } };
   var findCondition2 = { g: { $et: 1 } };
   testPara.testCL.update( updateCondition2, findCondition2 );

   //check result
   var expRecs2 = [{ a: -2147483639, b: -1, c: -2147483638.43211, d: { 0: { $decimal: "2" } }, e: { name: { firstName: "han", lastName: "meimei" } }, f: { name: { firstNumber: { $decimal: "1001" }, lastNumber: 2 } } },
   { a: 1, b: 2147483639, c: 2147483641.56789, d: [{ $decimal: "3" }, 2, 3], e: { name: { firstName: 100 } }, f: { name: { firstNumber: { $decimal: "1001" }, lastNumber: 2 } } },
   { a: { $numberLong: "-9223372036854775799" }, b: { $numberLong: "-9223372036854775801" }, c: 1.56789, d: [{ $decimal: "3" }, 2, 3], e: { name: { firstName: "han", lastName: "meimei" } }, f: { name: { firstNumber: 1000 } } },
   { a: 1, b: { $numberLong: "9223372036854775799" }, c: 124.56789, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } },
   { a: -1.7E+308, b: -1, c: -1.7E+308, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } },
   { a: 1.7E+308, b: 1.7E+308, c: 1.56789, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } },
   { a: 1, b: -1, c: 1.56789, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } },
   { a: 1, b: -1, c: 1.56789, d: { 0: { $decimal: "2" } }, e: { name: { firstName: 100 } }, f: { name: { firstNumber: 1000 } } },
   { g: 2, h: 1 }];
   checkResult( testPara.testCL, null, null, expRecs2, { _id: 1 } );

   //match nothing and update nothing
   var updateCondition3 = { $inc: { g: 1, h: 1 } };
   var findCondition3 = { g: { $et: 1 } };
   testPara.testCL.update( updateCondition3, findCondition3 );

   //check result
   checkResult( testPara.testCL, null, null, expRecs2, { _id: 1 } );
}
