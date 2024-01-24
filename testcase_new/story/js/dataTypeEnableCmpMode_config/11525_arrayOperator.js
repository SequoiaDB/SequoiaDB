/************************************
*@Description: set enablemixcmp=false;use array attribute operator,
               indexScan and tableScan have the same result;
*@author:      zhaoyu
*@createdate:  2017.5.19
*@testlinkCase: seqDB-11525
**************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_11525";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //create index
   commCreateIndex( dbcl, "a", { a: 1 } );
   commCreateIndex( dbcl, "b", { b: -1 } );

   var hintConf = [{ "": "a" }, { "": "b" }, { "": null }];
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
   { a: { b: 1 } },
   { a: [1, 20, 30] },
   { a: [20, 1, 30] },
   { a: null },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { b: 1 }];
   dbcl.insert( doc );

   //expand
   var findConf1 = { a: { $expand: 1 } };
   var expRecs1 = [{ a: 100 },
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
   { a: 1 },
   { a: 20 },
   { a: 30 },
   { a: 20 },
   { a: 1 },
   { a: 30 },
   { a: null },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { b: 1 }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   //returnMatch
   var findConf2 = { a: { $returnMatch: 0, $in: [1, 20] } };
   var expRecs2 = [{ a: [1, 20] },
   { a: [20, 1] }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   dbcl.remove();

   //insert data test expand
   var doc = [{ No: 1, b: 1, c: 1 },
   { No: 2, b: [2, 3, 4], c: [2, 3, 4] },
   { No: 3, b: [5, 6, [7, 8]], c: [5, 6, [7, 8]] },
   { No: 4, b: [] }];
   dbcl.insert( doc );

   var findCondition1 = { b: { $expand: 1 } };
   var expRecs1 = [{ No: 1, b: 1, c: 1 },
   { No: 2, b: 2, c: [2, 3, 4] },
   { No: 2, b: 3, c: [2, 3, 4] },
   { No: 2, b: 4, c: [2, 3, 4] },
   { No: 3, b: 5, c: [5, 6, [7, 8]] },
   { No: 3, b: 6, c: [5, 6, [7, 8]] },
   { No: 3, b: [7, 8], c: [5, 6, [7, 8]] },
   { No: 4, b: null }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition3 = { d: { $expand: 1 } };
   checkResult( dbcl, findCondition3, hintConf, sortConf, doc );

   var findCondition15 = { b: { $expand: 1, $in: [5, 10] } };
   var expRecs15 = [{ No: 3, b: 5, c: [5, 6, [7, 8]] },
   { No: 3, b: 6, c: [5, 6, [7, 8]] },
   { No: 3, b: [7, 8], c: [5, 6, [7, 8]] }];
   checkResult( dbcl, findCondition15, hintConf, sortConf, expRecs15 );

   var findCondition16 = { b: { $expand: 1, $in: [10] } };
   var expRecs16 = [];
   checkResult( dbcl, findCondition16, hintConf, sortConf, expRecs16 );

   var findCondition17 = { b: { $expand: 1, $in: [1, 3, [7, 8]] } };
   var expRecs17 = [{ No: 1, b: 1, c: 1 },
   { No: 2, b: 2, c: [2, 3, 4] },
   { No: 2, b: 3, c: [2, 3, 4] },
   { No: 2, b: 4, c: [2, 3, 4] },
   { No: 3, b: 5, c: [5, 6, [7, 8]] },
   { No: 3, b: 6, c: [5, 6, [7, 8]] },
   { No: 3, b: [7, 8], c: [5, 6, [7, 8]] }];
   checkResult( dbcl, findCondition17, hintConf, sortConf, expRecs17 );

   dbcl.remove();

   //insert data for test returnMatch
   var doc = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 3, b: 4 },
   { No: 4, b: [1, 2, 3, 4, 5] },
   { No: 5, b: [1, [[1, 2, 3], 3, 4], 3, 5, 2, 6, 1, 5, 7] },
   { No: 6, b: [] }];
   dbcl.insert( doc );

   //seqDB-10331
   var findCondition1 = { b: { $returnMatch: 0, $in: [1, 2, 3] } };
   var expRecs1 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [1, 2, 3] },
   { No: 5, b: [1, 3, 2, 1] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition2 = { b: { $returnMatch: 2, $in: [1, 2, 3] } };
   var expRecs2 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [3] },
   { No: 5, b: [2, 1] }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   var findCondition3 = { b: { $returnMatch: -2, $in: [1, 2, 3] } };
   var expRecs3 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [2, 3] },
   { No: 5, b: [2, 1] }];
   checkResult( dbcl, findCondition3, hintConf, sortConf, expRecs3 );

   var findCondition4 = { b: { $returnMatch: 4, $in: [1, 2, 3] } };
   var expRecs4 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: null },
   { No: 5, b: null }];
   checkResult( dbcl, findCondition4, hintConf, sortConf, expRecs4 );

   var findCondition5 = { b: { $returnMatch: -4, $in: [1, 2, 3] } };
   var expRecs5 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: null },
   { No: 5, b: [1, 3, 2, 1] }];
   checkResult( dbcl, findCondition5, hintConf, sortConf, expRecs5 );

   //seqDB-10334
   var findCondition8 = { c: { $returnMatch: 0, $in: [1, 2, 3] } };
   var expRecs8 = [];
   checkResult( dbcl, findCondition8, hintConf, sortConf, expRecs8 );

   //seqDB-10335
   var findCondition9 = { b: { $returnMatch: [-3, 2], $in: [1, 2, 3] } };
   var expRecs9 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [1, 2] },
   { No: 5, b: [3, 2] }];
   checkResult( dbcl, findCondition9, hintConf, sortConf, expRecs9 );

   var findCondition10 = { b: { $returnMatch: [-3, 4], $in: [1, 2, 3] } };
   var expRecs10 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [1, 2, 3] },
   { No: 5, b: [3, 2, 1] }];
   checkResult( dbcl, findCondition10, hintConf, sortConf, expRecs10 );

   var findCondition11 = { b: { $returnMatch: [2, 2], $in: [1, 2, 3] } };
   var expRecs11 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [3] },
   { No: 5, b: [2, 1] }];
   checkResult( dbcl, findCondition11, hintConf, sortConf, expRecs11 );

   var findCondition12 = { b: { $returnMatch: [-3, -2], $in: [1, 2, 3] } };
   var expRecs12 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [1, 2, 3] },
   { No: 5, b: [3, 2, 1] }];
   checkResult( dbcl, findCondition12, hintConf, sortConf, expRecs12 );

   var findCondition13 = { b: { $returnMatch: [-3, -4], $in: [1, 2, 3] } };
   var expRecs13 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [1, 2, 3] },
   { No: 5, b: [3, 2, 1] }];
   checkResult( dbcl, findCondition13, hintConf, sortConf, expRecs13 );

   var findCondition13 = { b: { $returnMatch: [2, 2], $in: [1, 2, 3] } };
   var expRecs13 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [3] },
   { No: 5, b: [2, 1] }];
   checkResult( dbcl, findCondition13, hintConf, sortConf, expRecs13 );

   var findCondition14 = { b: { $returnMatch: [3, 4], $in: [1, 2, 3] } };
   var expRecs14 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: null },
   { No: 5, b: [1] }];
   checkResult( dbcl, findCondition14, hintConf, sortConf, expRecs14 );

   var findCondition15 = { b: { $returnMatch: [-4, 4], $in: [1, 2, 3] } };
   var expRecs15 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: null },
   { No: 5, b: [1, 3, 2, 1] }];
   checkResult( dbcl, findCondition15, hintConf, sortConf, expRecs15 );

   var findCondition16 = { b: { $returnMatch: [0, 4], $in: [1, 2, 3] } };
   var expRecs16 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [1, 2, 3] },
   { No: 5, b: [1, 3, 2, 1] }];
   checkResult( dbcl, findCondition16, hintConf, sortConf, expRecs16 );

   var findCondition17 = { b: { $returnMatch: [0, -4], $in: [1, 2, 3] } };
   var expRecs17 = [{ No: 1, b: 1 },
   { No: 2, b: 2 },
   { No: 4, b: [1, 2, 3] },
   { No: 5, b: [1, 3, 2, 1] }];
   checkResult( dbcl, findCondition17, hintConf, sortConf, expRecs17 );

   //seqDB-10338
   var findCondition20 = { c: { $returnMatch: [0, 1], $in: [1, 2, 3] } };
   var expRecs20 = [];
   checkResult( dbcl, findCondition20, hintConf, sortConf, expRecs20 );

   //empty arr
   var findCondition21 = { c: { $returnMatch: [0, 1], $in: [] } };
   var expRecs21 = [];
   checkResult( dbcl, findCondition21, hintConf, sortConf, expRecs21 );
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}
