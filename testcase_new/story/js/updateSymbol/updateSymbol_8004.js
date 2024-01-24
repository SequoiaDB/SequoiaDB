/************************************
*@Description: update exist object ,use operator pull_all
*@author:      zhaoyu
*@createdate:  2016.5.19
**************************************/
main( test );
function test ()
{
   //clear environment
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data
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
   { object6: [200, [305, -299, 400, 1, 50, 1000], 400] },
   { object7: [200, [305, -299, [400, 1, 50], 1000], 400] },
   { object8: [200, [305, -299, 400, 1, 50, 1000], 400] }];
   dbcl.insert( doc1 );

   //update use pull_all exist object,no matches
   var updateCondition1 = {
      $pull_all: {
         object1: [10, 20],
         object2: [12, 400],
         object3: [12, 15, 99],
         object4: [100],
         object5: [false, "string", { $date: "2016-05-16" }],
         "object6.1": [-299, 400],
         "object7.1.2": [1, 50],
         "object8.1.2": [400]
      }
   };
   dbcl.update( updateCondition1 );

   //check result
   var expRecs1 = [{ object1: [-30] },
   { object2: 12 },
   { object3: [10, 30, -20, 80] },
   { object4: [200, [305, -299, 400], 400] },
   {
      object5: [-2147483640,
      { $numberLong: "-9223372036854775800" },
      { $decimal: "9223372036854775800" },
      { $oid: "573920accc332f037c000013" },
      { $timestamp: "2016-05-16-13.14.26.124233" },
      { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
      { $regex: "^z", $options: "i" },
         null]
   },
   { object6: [200, [305, 1, 50, 1000], 400] },
   { object7: [200, [305, -299, [400], 1000], 400] },
   { object8: [200, [305, -299, 400, 1, 50, 1000], 400] }];
   checkResult( dbcl, null, null, expRecs1, { _id: 1 } );

   //update use pull_all exist object,with matches
   var updateCondition2 = {
      $pull_all: {
         object1: [10, 20],
         object2: [12, 400],
         object3: [12, 15, 99],
         object4: [100],
         object5: [false, "string", { $date: "2016-05-16" }],
         "object6.1": [-299, 400],
         "object7.1.2": [1, 50],
         "object8.1": [400]
      }
   };
   var findCondition2 = { object8: { $exists: 1 } };
   dbcl.update( updateCondition2, findCondition2 );

   //check result
   var expRecs2 = [{ object1: [-30] },
   { object2: 12 },
   { object3: [10, 30, -20, 80] },
   { object4: [200, [305, -299, 400], 400] },
   {
      object5: [-2147483640,
      { $numberLong: "-9223372036854775800" },
      { $decimal: "9223372036854775800" },
      { $oid: "573920accc332f037c000013" },
      { $timestamp: "2016-05-16-13.14.26.124233" },
      { $binary: "aGVsbG8gd29ybGQ=", $type: "1" },
      { $regex: "^z", $options: "i" },
         null]
   },
   { object6: [200, [305, 1, 50, 1000], 400] },
   { object7: [200, [305, -299, [400], 1000], 400] },
   { object8: [200, [305, -299, 1, 50, 1000], 400] }];
   checkResult( dbcl, null, null, expRecs2, { _id: 1 } );
}