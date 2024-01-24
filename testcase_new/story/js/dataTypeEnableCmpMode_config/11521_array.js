/************************************
*@Description: set enablemixcmp=false;compare value set array,
               indexScan and tableScan have the same result;
*@author:      zhaoyu
*@createdate:  2017.5.19
*@testlinkCase: seqDB-11521
**************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_11521";
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
   { a: "aa" },
   { a: "ab" },
   { a: "b" },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { b: 1 } },
   { a: [1, 20, 30] },
   { a: [1, 20, 30] },
   { a: [20, 1, 30] },
   { a: [1, 20, 40] },
   { a: [1, 25, 10] },
   { a: [2, 30, 40] },
   { a: ["a", 60, 70] },
   { a: [0, -10, -20] },
   { a: [0, 30, 40] },
   { a: [1, 20] },
   { a: [[0, 20, 30], [1, 20, 30]] },
   { a: [[0, 20, 30], [2, 20, 30]] },
   { a: [[1, 20, 35], [1, 25, 30]] },
   { a: [[1, 20, 30], 50] },
   { a: [[1, 20, 30], 0] },
   { a: null },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { b: 1 }];
   dbcl.insert( doc );

   //gt
   var findConf1 = { a: { $gt: [1, 20, 30] } };
   var expRecs1 = [{ a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: [20, 1, 30] },
   { a: [1, 20, 40] },
   { a: [1, 25, 10] },
   { a: [2, 30, 40] },
   { a: ["a", 60, 70] },
   { a: [[0, 20, 30], [1, 20, 30]] },
   { a: [[0, 20, 30], [2, 20, 30]] },
   { a: [[1, 20, 35], [1, 25, 30]] },
   { a: [[1, 20, 30], 50] },
   { a: [[1, 20, 30], 0] },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   //gte
   var findConf2 = { a: { $gte: [1, 20, 30] } };
   var expRecs2 = [{ a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: [1, 20, 30] },
   { a: [1, 20, 30] },
   { a: [20, 1, 30] },
   { a: [1, 20, 40] },
   { a: [1, 25, 10] },
   { a: [2, 30, 40] },
   { a: ["a", 60, 70] },
   { a: [[0, 20, 30], [1, 20, 30]] },
   { a: [[0, 20, 30], [2, 20, 30]] },
   { a: [[1, 20, 35], [1, 25, 30]] },
   { a: [[1, 20, 30], 50] },
   { a: [[1, 20, 30], 0] },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   //lt
   var findConf3 = { a: { $lt: [1, 20, 30] } };
   var expRecs3 = [{ a: 100 },
   { a: 200 },
   { a: 100.23 },
   { a: { $decimal: "300" } },
   { a: "aa" },
   { a: "ab" },
   { a: "b" },
   { a: { b: 1 } },
   { a: [0, -10, -20] },
   { a: [0, 30, 40] },
   { a: [1, 20] },
   { a: [[0, 20, 30], [1, 20, 30]] },
   { a: [[0, 20, 30], [2, 20, 30]] },
   { a: null },
   { a: { $minKey: 1 } }];
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   //lte
   var findConf4 = { a: { $lte: [1, 20, 30] } };
   var expRecs4 = [{ a: 100 },
   { a: 200 },
   { a: 100.23 },
   { a: { $decimal: "300" } },
   { a: "aa" },
   { a: "ab" },
   { a: "b" },
   { a: { b: 1 } },
   { a: [1, 20, 30] },
   { a: [1, 20, 30] },
   { a: [0, -10, -20] },
   { a: [0, 30, 40] },
   { a: [1, 20] },
   { a: [[0, 20, 30], [1, 20, 30]] },
   { a: [[0, 20, 30], [2, 20, 30]] },
   { a: [[1, 20, 30], 50] },
   { a: [[1, 20, 30], 0] },
   { a: null },
   { a: { $minKey: 1 } }];
   checkResult( dbcl, findConf4, hintConf, sortConf, expRecs4 );

   //et,SEQUOIADBMAINSTREAM-2468
   var findConf5 = { a: { $et: [1, 20, 30] } };
   var expRecs5 = [{ a: [1, 20, 30] },
   { a: [1, 20, 30] },
   { a: [[0, 20, 30], [1, 20, 30]] },
   { a: [[1, 20, 30], 50] },
   { a: [[1, 20, 30], 0] },];
   checkResult( dbcl, findConf5, hintConf, sortConf, expRecs5 );

   //ne
   var findConf6 = { a: { $ne: [1, 20, 30] } };
   var expRecs6 = [{ a: 100 },
   { a: 200 },
   { a: 100.23 },
   { a: { $decimal: "300" } },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { b: 1 } },
   { a: [20, 1, 30] },
   { a: [1, 20, 40] },
   { a: [1, 25, 10] },
   { a: [2, 30, 40] },
   { a: ["a", 60, 70] },
   { a: [0, -10, -20] },
   { a: [0, 30, 40] },
   { a: [1, 20] },
   { a: [[0, 20, 30], [2, 20, 30]] },
   { a: [[1, 20, 35], [1, 25, 30]] },
   { a: null },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf6, hintConf, sortConf, expRecs6 );
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}
