/************************************
*@Description: update any object(exist or not exist) use operator pull
*@author:      zhaoyu
*@createdate:  2016.5.18
**************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert arr and common data   
   var doc1 = [{ object1: [10, -30, 20] },
   { object2: 12 },
   { object3: [10, 30, -20, 15, 99, 80] },
   { object4: [200, [305, -299, 400], 400] },
   {
      object5: [-2147483640,
      { $numberLong: "-9223372036854775800" },
      { $decimal: "9223372036854775800" },
         "string",
      { $oid: "573920accc332f037c000013" },
         false,
      { $date: "2016-05-16" },
      { $timestamp: "2016-05-16-13.14.26.124233" },
      { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
      { $regex: "^z", $options: "i" },
         null]
   },
   {
      object8: [-2147483640,
      { $numberLong: "-9223372036854775800" },
      { $decimal: "9223372036854775800" },
         "string",
      { $oid: "573920accc332f037c000013" },
         false,
      { $date: "2016-05-16" },
      { $timestamp: "2016-05-16-13.14.26.124233" },
      { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
      { $regex: "^z", $options: "i" },
         null]
   },
   { object9: [200, [305, 299, 400], 100] },
   { object10: [200, [305, 299, 400], 100] },
   { object11: [10, 30, 20] }];
   dbcl.insert( doc1 );

   //update use pull,no matches
   var updateCondition1 = {
      $pull: {
         object1: -30, object3: 20, object8: false,
         "object4.1": 400, "object10.1": 200,
         object7: 100, "object11.1": 20, "object12.0": 12,
         object2: 12
      }
   };
   dbcl.update( updateCondition1 );

   //check result
   var expRecs1 = [{ object1: [10, 20] },
   { object2: 12 },
   { object3: [10, 30, -20, 15, 99, 80] },
   { object4: [200, [305, -299], 400] },
   {
      object5: [-2147483640,
      { $numberLong: "-9223372036854775800" },
      { $decimal: "9223372036854775800" },
         "string",
      { $oid: "573920accc332f037c000013" },
         false,
      { $date: "2016-05-16" },
      { $timestamp: "2016-05-16-13.14.26.124233" },
      { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
      { $regex: "^z", $options: "i" },
         null]
   },
   {
      object8: [-2147483640,
      { $numberLong: "-9223372036854775800" },
      { $decimal: "9223372036854775800" },
         "string",
      { $oid: "573920accc332f037c000013" },
      { $date: "2016-05-16" },
      { $timestamp: "2016-05-16-13.14.26.124233" },
      { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
      { $regex: "^z", $options: "i" },
         null]
   },
   { object9: [200, [305, 299, 400], 100] },
   { object10: [200, [305, 299, 400], 100] },
   { object11: [10, 30, 20] }];
   checkResult( dbcl, null, null, expRecs1, { _id: 1 } );

   //update use pull,with matches
   var updateCondition2 = {
      $pull: {
         object1: -30, object3: -20, object8: false,
         "object4.1": 400, "object10.1": 200,
         object7: 100, "object11.1": 20, "object12.0": 12,
         object2: 12,
         object5: "string"
      }
   };
   var findCondition2 = { object5: { $exists: 1 } };
   dbcl.update( updateCondition2, findCondition2 );

   //check result
   var expRecs2 = [{ object1: [10, 20] },
   { object2: 12 },
   { object3: [10, 30, -20, 15, 99, 80] },
   { object4: [200, [305, -299], 400] },
   {
      object5: [-2147483640,
      { $numberLong: "-9223372036854775800" },
      { $decimal: "9223372036854775800" },
      { $oid: "573920accc332f037c000013" },
         false,
      { $date: "2016-05-16" },
      { $timestamp: "2016-05-16-13.14.26.124233" },
      { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
      { $regex: "^z", $options: "i" },
         null]
   },
   {
      object8: [-2147483640,
      { $numberLong: "-9223372036854775800" },
      { $decimal: "9223372036854775800" },
         "string",
      { $oid: "573920accc332f037c000013" },
      { $date: "2016-05-16" },
      { $timestamp: "2016-05-16-13.14.26.124233" },
      { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
      { $regex: "^z", $options: "i" },
         null]
   },
   { object9: [200, [305, 299, 400], 100] },
   { object10: [200, [305, 299, 400], 100] },
   { object11: [10, 30, 20] }];
   checkResult( dbcl, null, null, expRecs2, { _id: 1 } );
}