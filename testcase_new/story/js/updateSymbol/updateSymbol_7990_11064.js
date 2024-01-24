/************************************
*@Description: seqDB-7990:匹配不到记录，upsert使用set更新符
               seqDB-11064:upsert不存在的记录，匹配条件使用$or下只包含单条件
*@author:      zhaoyu
*@createdate:  2016.5.17
*@update:      2017.2.17/zhaoyu
**************************************/
testConf.clName = COMMCLNAME + "_set7990";
main( test );

function test ( testPara )
{
   //insert object
   var doc = { a: 1 };
   testPara.testCL.insert( doc );

   //upsert any object when match nothing,use matches and
   var upsertCondition1 = {
      $set: {
         object1: 2147483640,
         object2: { $numberLong: "-9223372036854775800" },
         object3: { $decimal: "9223372036854775800" },
         object4: -1.7E+308,
         object5: "string",
         object6: { $oid: "573920accc332f037c000013" },
         object7: false,
         object8: { $date: "2016-05-16" },
         object9: { $timestamp: "2016-05-16-13.14.26.124233" },
         object10: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
         object11: { $regex: "^z", $options: "i" },
         object12: { name: "hanmeimei" },
         object13: [12, 34, 36],
         object14: null
      }
   };
   var findCondition1 = {
      $and: [{ object1: { $et: { $decimal: "2" } } },
      { object13: { $all: [10, 20, 30] } },
      { c: { $gt: 100 } },
      { d: 1000 },
      { "e.name.firstName": null }]
   };
   testPara.testCL.upsert( upsertCondition1, findCondition1 );

   //check result
   var expRecs1 = [{
      object1: 2147483640,
      object2: { $numberLong: "-9223372036854775800" },
      object3: { $decimal: "9223372036854775800" },
      object4: -1.7E+308,
      object5: "string",
      object6: { $oid: "573920accc332f037c000013" },
      object7: false,
      object8: { $date: "2016-05-16" },
      object9: { $timestamp: "2016-05-16-13.14.26.124233" },
      object10: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
      object11: { $regex: "^z", $options: "i" },
      object12: { name: "hanmeimei" },
      object13: [12, 34, 36],
      object14: null,
      d: 1000,
      e: { name: { firstName: null } }
   },
   { a: 1 }];
   checkResult( testPara.testCL, null, null, expRecs1, { a: 1 } );

   //upsert any object when match nothing,use matches or
   var upsertCondition2 = {
      $set: {
         object1: 2147483640,
         object2: { $numberLong: "-9223372036854775800" },
         object3: { $decimal: "9223372036854775800" },
         object4: -1.7E+308,
         object5: "string",
         object6: { $oid: "573920accc332f037c000013" },
         object7: false,
         object8: { $date: "2016-05-16" },
         object9: { $timestamp: "2016-05-16-13.14.26.124233" },
         object10: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
         object11: { $regex: "^z", $options: "i" },
         object12: { name: "hanmeimei" },
         object13: [12, 34, 36],
         object14: null
      }
   };
   var findCondition2 = {
      $or: [{ object1: { $et: { $decimal: "2" } } },
      { object13: { $all: [10, 20, 30] } },
      { c: { $gt: 100 } },
      { d: 1001 },
      { "e.name.firstName": "han" }]
   };
   testPara.testCL.upsert( upsertCondition2, findCondition2 );

   //check result
   var expRecs2 = [{
      object1: 2147483640,
      object2: { $numberLong: "-9223372036854775800" },
      object3: { $decimal: "9223372036854775800" },
      object4: -1.7E+308,
      object5: "string",
      object6: { $oid: "573920accc332f037c000013" },
      object7: false,
      object8: { $date: "2016-05-16" },
      object9: { $timestamp: "2016-05-16-13.14.26.124233" },
      object10: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
      object11: { $regex: "^z", $options: "i" },
      object12: { name: "hanmeimei" },
      object13: [12, 34, 36],
      object14: null,
      d: 1000,
      e: { name: { firstName: null } }
   },
   {
      object1: 2147483640,
      object2: { $numberLong: "-9223372036854775800" },
      object3: { $decimal: "9223372036854775800" },
      object4: -1.7E+308,
      object5: "string",
      object6: { $oid: "573920accc332f037c000013" },
      object7: false,
      object8: { $date: "2016-05-16" },
      object9: { $timestamp: "2016-05-16-13.14.26.124233" },
      object10: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
      object11: { $regex: "^z", $options: "i" },
      object12: { name: "hanmeimei" },
      object13: [12, 34, 36],
      object14: null
   },
   { a: 1 }];
   checkResult( testPara.testCL, null, null, expRecs2, { a: 1 } );

   //delete all data
   testPara.testCL.remove();

   //upsert any object when match nothing,use matches not
   var upsertCondition3 = {
      $set: {
         object1: 2147483640,
         object2: { $numberLong: "-9223372036854775800" },
         object3: { $decimal: "9223372036854775800" },
         object4: -1.7E+308,
         object5: "string",
         object6: { $oid: "573920accc332f037c000013" },
         object7: false,
         object8: { $date: "2016-05-16" },
         object9: { $timestamp: "2016-05-16-13.14.26.124233" },
         object10: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
         object11: { $regex: "^z", $options: "i" },
         object12: { name: "hanmeimei" },
         object13: [12, 34, 36],
         object14: null
      }
   };
   var findCondition3 = {
      $not: [{ object1: { $et: { $decimal: "2" } } },
      { object13: { $all: [10, 20, 30] } },
      { c: { $gt: 100 } },
      { d: 1001 },
      { "e.name.firstName": "han" }]
   };
   testPara.testCL.upsert( upsertCondition3, findCondition3 );

   //check result
   var expRecs3 = [{
      object1: 2147483640,
      object2: { $numberLong: "-9223372036854775800" },
      object3: { $decimal: "9223372036854775800" },
      object4: -1.7E+308,
      object5: "string",
      object6: { $oid: "573920accc332f037c000013" },
      object7: false,
      object8: { $date: "2016-05-16" },
      object9: { $timestamp: "2016-05-16-13.14.26.124233" },
      object10: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
      object11: { $regex: "^z", $options: "i" },
      object12: { name: "hanmeimei" },
      object13: [12, 34, 36],
      object14: null
   }];
   checkResult( testPara.testCL, null, null, expRecs3, { a: 1 } );

   //delete all data
   testPara.testCL.remove();

   //upsert use or has one condition,2017.2.17/zhaoyu/seqDB-11064
   var upsertCondition4 = { $set: { a: 1 } };
   var findCondition4 = { $or: [{ b: 1 }] };
   testPara.testCL.upsert( upsertCondition4, findCondition4 );
   var expRecs4 = [{ a: 1, b: 1 }];
   checkResult( testPara.testCL, null, null, expRecs4, { a: 1 } );

   var upsertCondition5 = { $set: { a: 2 } };
   var findCondition5 = { $or: [{ b: { $et: 2 } }] };
   testPara.testCL.upsert( upsertCondition5, findCondition5 );
   var expRecs5 = [{ a: 1, b: 1 },
   { a: 2, b: 2 }];
   checkResult( testPara.testCL, null, null, expRecs5, { a: 1 } );

   var upsertCondition6 = { $set: { a: 3 } };
   var findCondition6 = { $or: [{ b: { $all: [3] } }] };
   testPara.testCL.upsert( upsertCondition6, findCondition6 );
   var expRecs6 = [{ a: 1, b: 1 },
   { a: 2, b: 2 },
   { a: 3, b: [3] }];
   checkResult( testPara.testCL, null, null, expRecs6, { a: 1 } );
}
