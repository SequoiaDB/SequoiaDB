/************************************
*@Description: seqDB-7984:更新符inc的值为非数值
*@author:      zhaoyu
*@createdate:  2016.5.16
**************************************/
testConf.clName = COMMCLNAME + "_inc7984";
main( test );

function test ( testPara )
{
   //insert all kind of data   
   var doc1 = [{ a: -2147483640, b: -2147483640, c: -2147483640 },
   { a: 2147483640, b: 2147483640, c: 2147483640 },
   { a: { $numberLong: "-9223372036854775800" }, b: { $numberLong: "-9223372036854775800" }, c: { $numberLong: "-9223372036854775800" } },
   { a: { $numberLong: "9223372036854775800" }, b: { $numberLong: "9223372036854775800" }, c: { $numberLong: "9223372036854775800" } },
   { a: -1.7E+308, b: -1.7E+308, c: -1.7E+308 },
   { a: 1.7E+308, b: 1.7E+308, c: 1.7E+308 },
   { a: -4.9E-324, b: -4.9E-324, c: -4.9E-324 },
   { a: 4.9E-324, b: 4.9E-324, c: 4.9E-324 },
   { a: "string", b: "string", c: "string" },
   { a: { $oid: "573920accc332f037c000013" }, b: { $oid: "573920accc332f037c000013" }, c: { $oid: "573920accc332f037c000013" } },
   { a: false, b: false, c: false },
   { a: true, b: true, c: true },
   { a: { $date: "2016-05-16" }, b: { $date: "2016-05-16" }, c: { $date: "2016-05-16" } },
   { a: { $timestamp: "2016-05-16-13.14.26.124233" }, b: { $timestamp: "2016-05-16-13.14.26.124233" }, c: { $timestamp: "2016-05-16-13.14.26.124233" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, b: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, c: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" }, b: { $regex: "^z", $options: "i" }, c: { $regex: "^z", $options: "i" } },
   { a: { name: "hanmeimei" }, b: { name: "hanmeimei" }, c: { name: "hanmeimei" } },
   { a: ["b", 0], b: ["b", 0], c: ["b", 0] },
   { a: null, b: null, c: null }];
   testPara.testCL.insert( doc1 );

   //update use $inc a non condition
   var updateCondition1 = { $inc: {} };
   testPara.testCL.update( updateCondition1 )

   //check result
   checkResult( testPara.testCL, null, null, doc1, { _id: 1 } );

   //update use $inc a non numberic
   var updateCondition2 = { $inc: { a: 1, b: "string", c: 1 } };
   invalidDataUpdateCheckResult( testPara.testCL, updateCondition2, -6 );

   //check result
   checkResult( testPara.testCL, null, null, doc1, { _id: 1 } );
}
