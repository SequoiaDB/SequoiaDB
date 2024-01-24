/************************************
*@Description: seqDB-7982:inc更新符的数值运算功能测试
*@author:      zhaoyu
*@createdate:  2016.5.16
**************************************/

testConf.clName = COMMCLNAME + "_inc7982";
main( test );
function test ( testPara )
{
   //insert all kind of type data
   var doc1 = [{ a: -2147483640 },
   { a: 2147483640 },
   { a: { $numberLong: "-9223372036854775800" } },
   { a: { $numberLong: "9223372036854775800" } },
   { a: -1.7E+308 },
   { a: 1.7E+308 },
   { a: -4.9E-324 },
   { a: 4.9E-324 },
   { a: "string" },
   { a: { $oid: "573920accc332f037c000013" } },
   { a: false },
   { a: true },
   { a: { $date: "2016-05-16" } },
   { a: { $timestamp: "2016-05-16-13.14.26.124233" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: { name: "hanmeimei" } },
   { a: ["b", 0] },
   { a: null }];
   testPara.testCL.insert( doc1 );

   //update use $inc a negative number,in range,then check
   var updateCondition1 = { $inc: { a: -8 } };
   testPara.testCL.update( updateCondition1 )

   //check result
   var expRecs1 = [{ a: -2147483648 },
   { a: 2147483632 },
   { a: { $numberLong: "-9223372036854775808" } },
   { a: { $numberLong: "9223372036854775792" } },
   { a: -1.7E+308 },
   { a: 1.7E+308 },
   { a: -8 },
   { a: -8 },
   { a: "string" },
   { a: { $oid: "573920accc332f037c000013" } },
   { a: false },
   { a: true },
   { a: { $date: "2016-05-16" } },
   { a: { $timestamp: "2016-05-16-13.14.26.124233" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: { name: "hanmeimei" } },
   { a: ["b", 0] },
   { a: null }];
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );

   //update use $inc a positive number,in range,then check
   var updateCondition2 = { $inc: { a: 15 } };
   testPara.testCL.update( updateCondition2 )

   //check result
   var expRecs2 = [{ a: -2147483633 },
   { a: 2147483647 },
   { a: { $numberLong: "-9223372036854775793" } },
   { a: { $numberLong: "9223372036854775807" } },
   { a: -1.7E+308 },
   { a: 1.7E+308 },
   { a: 7 },
   { a: 7 },
   { a: "string" },
   { a: { $oid: "573920accc332f037c000013" } },
   { a: false },
   { a: true },
   { a: { $date: "2016-05-16" } },
   { a: { $timestamp: "2016-05-16-13.14.26.124233" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: { name: "hanmeimei" } },
   { a: ["b", 0] },
   { a: null }];
   checkResult( testPara.testCL, null, null, expRecs2, { _id: 1 } );
}

