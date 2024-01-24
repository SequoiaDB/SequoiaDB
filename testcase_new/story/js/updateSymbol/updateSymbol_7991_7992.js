/************************************
*@Description:  seqDB-7991:update使用unset更新符更新不存在的对象
                seqDB-7992:update使用unset更新符更新已存在的对象
*@author:      zhaoyu
*@createdate:  2016.5.17
**************************************/

testConf.clName = COMMCLNAME + "_unset7991";
main( test );

function test ( testPara )
{
   //insert all kind of data   
   var doc1 = [{ b: -2147483640, c: -2147483640, d: 4096 },
   { a: { $numberLong: "-9223372036854775800" }, c: { $numberLong: "-9223372036854775800" }, d: [10, 100, 1000] },
   { a: { $decimal: "9223372036854775800" }, b: { $decimal: "9223372036854775800" }, e: { name: { firstName: "han", lastName: "meimei" } } },
   { a: -1.7E+308, b: -1.7E+308, c: -1.7E+308 },
   { a: "string", b: "string", c: "string" },
   { a: { $oid: "573920accc332f037c000013" }, b: { $oid: "573920accc332f037c000013" }, c: { $oid: "573920accc332f037c000013" } },
   { a: false, b: true, c: false },
   { a: { $date: "2016-05-16" }, b: { $date: "2016-05-16" }, c: { $date: "2016-05-16" } },
   { a: { $timestamp: "2016-05-16-13.14.26.124233" }, b: { $timestamp: "2016-05-16-13.14.26.124233" }, c: { $timestamp: "2016-05-16-13.14.26.124233" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, b: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, c: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" }, b: { $regex: "^z", $options: "i" }, c: { $regex: "^z", $options: "i" } },
   { a: { name: "hanmeimei" }, b: { name: "hanmeimei" }, c: { name: "hanmeimei" } },
   { a: [10, 100, 1000], b: [11, 101, 1001], f: [12, 102, 1002] },
   { a: null, b: null, c: null }];
   testPara.testCL.insert( doc1 );

   //update use unset,no matches
   var updateCondition1 = { $unset: { a: "", b: "", c: "", "d.2": "", "d.3": "", "e.name.firstName": "", f: "", "e.name.firstName1": "" } };
   testPara.testCL.update( updateCondition1 )

   //check result
   var expRecs1 = [{ d: 4096 },
   { d: [10, 100, null] },
   { e: { name: { lastName: "meimei" } } },
   {},
   {},
   {},
   {},
   {},
   {},
   {},
   {},
   {},
   {},
   {}];
   checkResult( testPara.testCL, null, null, expRecs1, { _id: 1 } );

   //update use unset,with matches
   var updateCondition2 = { $unset: { d: "" } };
   var findCondition2 = { d: { $type: 1, $et: 4 } };
   testPara.testCL.update( updateCondition2, findCondition2 )

   //check result
   var expRecs2 = [{ d: 4096 },
   {},
   { e: { name: { lastName: "meimei" } } },
   {},
   {},
   {},
   {},
   {},
   {},
   {},
   {},
   {},
   {},
   {}];
   checkResult( testPara.testCL, null, null, expRecs2, { _id: 1 } );
}

