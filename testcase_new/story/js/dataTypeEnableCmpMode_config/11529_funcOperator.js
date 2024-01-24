/************************************
*@Description: set enablemixcmp=false;use function operator,
               indexScan and tableScan have the same result;
*@author:      zhaoyu
*@createdate:  2017.5.19
*@testlinkCase: seqDB-11529
**************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_11529";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //create index
   commCreateIndex( dbcl, "a", { a: 1 } );

   var hintConf = [{ "": "a" }, { "": null }];
   var sortConf = { _id: 1 };

   //insert all type data 
   var doc = [{ a: 100 },
   { a: { $numberLong: "200" } },
   { a: 100.23 },
   { a: { $decimal: "300" } },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { b: 1, c: 1, d: 1 } },
   { a: { b: 1, c: 1 } },
   { a: [1, 20] },
   { a: [20, 1, 30] },
   { a: null },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { b: 1 }];
   dbcl.insert( doc );

   //type
   var findConf1 = { a: { $type: 1, $et: 16 } };
   var expRecs1 = [{ a: 100 }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   var findConf2 = { a: { $type: 1, $gt: 16 } };
   var expRecs2 = [{ a: 200 },
   { a: { $decimal: "300" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   var findConf3 = { a: { $type: 1, $gte: 16 } };
   var expRecs3 = [{ a: 100 },
   { a: 200 },
   { a: { $decimal: "300" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   var findConf4 = { a: { $type: 1, $lt: 16 } };
   var expRecs4 = [{ a: 100.23 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { b: 1, c: 1, d: 1 } },
   { a: { b: 1, c: 1 } },
   { a: [1, 20] },
   { a: [20, 1, 30] },
   { a: null },
   { a: { $minKey: 1 } }];
   checkResult( dbcl, findConf4, hintConf, sortConf, expRecs4 );

   var findConf5 = { a: { $type: 1, $lte: 16 } };
   var expRecs5 = [{ a: 100 },
   { a: 100.23 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { b: 1, c: 1, d: 1 } },
   { a: { b: 1, c: 1 } },
   { a: [1, 20] },
   { a: [20, 1, 30] },
   { a: null },
   { a: { $minKey: 1 } }];
   checkResult( dbcl, findConf5, hintConf, sortConf, expRecs5 );

   //size
   var findConf6 = { a: { $size: 1, $et: 3 } };
   var expRecs6 = [{ a: { b: 1, c: 1, d: 1 } },
   { a: [20, 1, 30] }];
   checkResult( dbcl, findConf6, hintConf, sortConf, expRecs6 );

   var findConf7 = { a: { $size: 1, $gt: 2 } };
   var expRecs7 = [{ a: { b: 1, c: 1, d: 1 } },
   { a: [20, 1, 30] }];
   checkResult( dbcl, findConf7, hintConf, sortConf, expRecs7 );

   var findConf8 = { a: { $size: 1, $gte: 2 } };
   var expRecs8 = [{ a: { b: 1, c: 1, d: 1 } },
   { a: { b: 1, c: 1 } },
   { a: [1, 20] },
   { a: [20, 1, 30] }];
   checkResult( dbcl, findConf8, hintConf, sortConf, expRecs8 );

   var findConf9 = { a: { $size: 1, $lt: 3 } };
   var expRecs9 = [{ a: { b: 1, c: 1 } },
   { a: [1, 20] }];
   checkResult( dbcl, findConf9, hintConf, sortConf, expRecs9 );

   var findConf10 = { a: { $size: 1, $lte: 3 } };
   var expRecs10 = [{ a: { b: 1, c: 1, d: 1 } },
   { a: { b: 1, c: 1 } },
   { a: [1, 20] },
   { a: [20, 1, 30] }];
   checkResult( dbcl, findConf10, hintConf, sortConf, expRecs10 );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}
