/************************************
*@Description: update any object(exist or not exist) use operator set
*@author:      zhaoyu
*@createdate:  2016.5.17
*@Modifier:    2020-08-11 Zixian Yan
**************************************/
testConf.clName = COMMCLNAME + "_7988_7989";
main( test );

function test ( testPara )
{

   var dbCL = testPara.testCL;

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
   { a: ["b", 0], b: ["b", 0], c: ["b", 0] },
   { a: null, b: null, c: null }];
   dbCL.insert( doc1 );

   //update use set,no matches
   var updateCondition1 = { $set: { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, "d.0": 25, "e.name.firstName": null } };
   dbCL.update( updateCondition1 );

   //check result
   var expRecs1 = [{ a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: 4096, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: [25, 100, 1000], e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null, lastName: "meimei" } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } }];
   checkResult( dbCL, null, null, expRecs1, { _id: 1 } );

   //update use set,with matches
   var updateCondition2 = { $set: { a: { $regex: "^z", $options: "i" }, b: { $timestamp: "2016-05-16-13.14.26.124233" }, c: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, "d.1": 35, "e.name.firstName": true, f: 56 } };
   var findCondition2 = { d: { $type: 1, $et: 4 } };
   dbCL.update( updateCondition2, findCondition2 );

   //check result
   var expRecs2 = [{ a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: 4096, e: { name: { firstName: null } } },
   { a: { $regex: "^z", $options: "i" }, b: { $timestamp: "2016-05-16-13.14.26.124233" }, c: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, d: [25, 35, 1000], e: { name: { firstName: true } }, f: 56 },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null, lastName: "meimei" } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } },
   { a: 123, b: "veryHappy", c: { $date: "2016-05-17" }, d: { 0: 25 }, e: { name: { firstName: null } } }];
   checkResult( dbCL, null, null, expRecs2, { _id: 1 } );

   matches_isnull_feildExistOrNot_update( dbCL );

}

function matches_isnull_feildExistOrNot_update ( cl )
{
   cl.remove();
   var docList = [{ b: 1 }, { a: null, b: 2 }, { a: 1, b: 3 }];
   cl.insert( docList );

   cl.update( { $set: { b: -5 } }, { a: { $isnull: 1 } } );
   cl.update( { $set: { b: 5 } }, { a: { $isnull: 0 } } );

   var expectResult1 = [{ b: -5 }, { a: null, b: -5 }];
   var expectResult2 = [{ a: 1, b: 5 }];

   var record1 = cl.find( { a: { $isnull: 1 } } );
   var record2 = cl.find( { a: { $isnull: 0 } } );

   checkRec( record1, expectResult1 );
   checkRec( record2, expectResult2 );
}
