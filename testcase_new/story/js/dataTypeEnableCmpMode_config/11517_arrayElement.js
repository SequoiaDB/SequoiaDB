/************************************
*@Description: set enablemixcmp=false;compare value set array element,
               indexScan and tableScan have the same result;
*@author:      zhaoyu
*@createdate:  2017.5.19
*@testlinkCase: seqDB-11517
**************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_11517";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //create index
   commCreateIndex( dbcl, "a", { a: 1 } );
   commCreateIndex( dbcl, "a1", { "a.1": 1 } );

   var hintConf = [{ "": "a" }, { "": null }, { "": "a1" }];
   var sortConf = { _id: 1 };

   //insert all type data 
   var doc = [{ a: { $minKey: 1 } },
   { b: 1 },
   { a: null },
   { a: 2 }, { a: 3 }, { a: 4 },
   { a: "aa" },
   { a: [-1, -2, -3] },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [3, 4, 5] },
   { a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { a: { 0: -1, 1: -2, 2: -3 } },
   { a: { 0: 1, 1: 2, 2: 3 } },
   { a: { 0: 2, 1: 1, 2: 3 } },
   { a: { 0: 2, 1: 3, 2: 4 } },
   { a: { 0: 3, 1: 4, 2: 5 } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   dbcl.insert( doc );

   //gt
   var findCondition1 = { a: { $gt: 2 } };
   var expRecs1 = [{ a: 3 }, { a: 4 },
   { a: "aa" },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [3, 4, 5] },
   { a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { a: { 0: -1, 1: -2, 2: -3 } },
   { a: { 0: 1, 1: 2, 2: 3 } },
   { a: { 0: 2, 1: 1, 2: 3 } },
   { a: { 0: 2, 1: 3, 2: 4 } },
   { a: { 0: 3, 1: 4, 2: 5 } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$gte:2
   var findCondition2 = { a: { $gte: 2 } };
   var expRecs2 = [{ a: 2 }, { a: 3 }, { a: 4 },
   { a: "aa" },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [3, 4, 5] },
   { a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { a: { 0: -1, 1: -2, 2: -3 } },
   { a: { 0: 1, 1: 2, 2: 3 } },
   { a: { 0: 2, 1: 1, 2: 3 } },
   { a: { 0: 2, 1: 3, 2: 4 } },
   { a: { 0: 3, 1: 4, 2: 5 } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   //$lt:2
   var findCondition3 = { a: { $lt: 2 } };
   var expRecs3 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: [-1, -2, -3] },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] }];
   checkResult( dbcl, findCondition3, hintConf, sortConf, expRecs3 );

   //$lte:2
   var findCondition4 = { a: { $lte: 2 } };
   var expRecs4 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 2 },
   { a: [-1, -2, -3] },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] }];
   checkResult( dbcl, findCondition4, hintConf, sortConf, expRecs4 );

   //mod
   var findCondition5 = { a: { $mod: [2, 1] } };
   var expRecs5 = [{ a: 3 },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [3, 4, 5] }];
   checkResult( dbcl, findCondition5, hintConf, sortConf, expRecs5 );

   //et
   var findCondition6 = { a: { $et: 2 } };
   var expRecs6 = [{ a: 2 },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] }];
   checkResult( dbcl, findCondition6, hintConf, sortConf, expRecs6 );

   //ne
   var findCondition7 = { a: { $ne: 2 } };
   var expRecs7 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 3 }, { a: 4 },
   { a: "aa" },
   { a: [-1, -2, -3] },
   { a: [3, 4, 5] },
   { a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { a: { 0: -1, 1: -2, 2: -3 } },
   { a: { 0: 1, 1: 2, 2: 3 } },
   { a: { 0: 2, 1: 1, 2: 3 } },
   { a: { 0: 2, 1: 3, 2: 4 } },
   { a: { 0: 3, 1: 4, 2: 5 } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findCondition7, hintConf, sortConf, expRecs7 );

   //in
   var findCondition8 = { a: { $in: [2, 1, { 1: 1 }] } };
   var expRecs8 = [{ a: 2 },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] }];
   checkResult( dbcl, findCondition8, hintConf, sortConf, expRecs8 );

   //all
   var findCondition9 = { a: { $all: [2] } };
   var expRecs9 = [{ a: 2 },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] }];
   checkResult( dbcl, findCondition9, hintConf, sortConf, expRecs9 );

   //"a.1" query
   var hintConf1 = [{ "": "a" }, { "": null }];
   var hintConf2 = [{ "": "a1" }];

   //$gt:2
   var findCondition5 = { "a.1": { $gt: 2 } };
   var expRecs5 = [{ a: [2, 3, 4] },
   { a: [3, 4, 5] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { a: { 0: 2, 1: 3, 2: 4 } },
   { a: { 0: 3, 1: 4, 2: 5 } }];
   checkResult( dbcl, findCondition5, hintConf1, sortConf, expRecs5 );

   var expRecs5 = [{ a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { a: { 0: 2, 1: 3, 2: 4 } },
   { a: { 0: 3, 1: 4, 2: 5 } }];
   checkResult( dbcl, findCondition5, hintConf2, sortConf, expRecs5 );

   //$gte:2
   var findCondition6 = { "a.1": { $gte: 2 } };
   var expRecs6 = [{ a: [1, 2, 3] },
   { a: [2, 3, 4] },
   { a: [3, 4, 5] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { a: { 0: 1, 1: 2, 2: 3 } },
   { a: { 0: 2, 1: 3, 2: 4 } },
   { a: { 0: 3, 1: 4, 2: 5 } }];
   checkResult( dbcl, findCondition6, hintConf1, sortConf, expRecs6 );

   var expRecs6 = [{ a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { a: { 0: 1, 1: 2, 2: 3 } },
   { a: { 0: 2, 1: 3, 2: 4 } },
   { a: { 0: 3, 1: 4, 2: 5 } }];
   checkResult( dbcl, findCondition6, hintConf2, sortConf, expRecs6 );

   //$lt:2
   var findCondition7 = { "a.1": { $lt: 2 } };
   var expRecs7 = [{ a: [-1, -2, -3] },
   { a: [2, 1, 3] },
   { a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: { 0: -1, 1: -2, 2: -3 } },
   { a: { 0: 2, 1: 1, 2: 3 } }];
   checkResult( dbcl, findCondition7, hintConf1, sortConf, expRecs7 );

   var expRecs7 = [{ a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: { 0: -1, 1: -2, 2: -3 } },
   { a: { 0: 2, 1: 1, 2: 3 } }];
   checkResult( dbcl, findCondition7, hintConf2, sortConf, expRecs7 );

   //$lte:2
   var findCondition8 = { "a.1": { $lte: 2 } };
   var expRecs8 = [{ a: [-1, -2, -3] },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: { 0: -1, 1: -2, 2: -3 } },
   { a: { 0: 1, 1: 2, 2: 3 } },
   { a: { 0: 2, 1: 1, 2: 3 } }];
   checkResult( dbcl, findCondition8, hintConf1, sortConf, expRecs8 );

   var expRecs8 = [{ a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: { 0: -1, 1: -2, 2: -3 } },
   { a: { 0: 1, 1: 2, 2: 3 } },
   { a: { 0: 2, 1: 1, 2: 3 } }];
   checkResult( dbcl, findCondition8, hintConf2, sortConf, expRecs8 );

   //mod
   var findCondition5 = { "a.1": { $mod: [2, 1] } };
   var expRecs5 = [{ a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: { 0: 2, 1: 1, 2: 3 } },
   { a: { 0: 2, 1: 3, 2: 4 } }];
   checkResult( dbcl, findCondition5, hintConf1, sortConf, expRecs5 );

   var expRecs5 = [{ a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: { 0: 2, 1: 1, 2: 3 } },
   { a: { 0: 2, 1: 3, 2: 4 } }];
   checkResult( dbcl, findCondition5, hintConf2, sortConf, expRecs5 );

   //et
   var findCondition6 = { "a.1": { $et: 2 } };
   var expRecs6 = [{ a: [1, 2, 3] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: { 0: 1, 1: 2, 2: 3 } }];
   checkResult( dbcl, findCondition6, hintConf1, sortConf, expRecs6 );

   var expRecs6 = [{ a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: { 0: 1, 1: 2, 2: 3 } }];
   checkResult( dbcl, findCondition6, hintConf2, sortConf, expRecs6 );

   //ne
   var findCondition5 = { "a.1": { $ne: 2 } };
   var expRecs5 = [{ a: [-1, -2, -3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [3, 4, 5] },
   { a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { a: { 0: -1, 1: -2, 2: -3 } },
   { a: { 0: 2, 1: 1, 2: 3 } },
   { a: { 0: 2, 1: 3, 2: 4 } },
   { a: { 0: 3, 1: 4, 2: 5 } }];
   checkResult( dbcl, findCondition5, hintConf1, sortConf, expRecs5 );

   var expRecs5 = [{ a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { a: { 0: -1, 1: -2, 2: -3 } },
   { a: { 0: 2, 1: 1, 2: 3 } },
   { a: { 0: 2, 1: 3, 2: 4 } },
   { a: { 0: 3, 1: 4, 2: 5 } }];
   checkResult( dbcl, findCondition5, hintConf2, sortConf, expRecs5 );

   //in
   var findCondition5 = { "a.1": { $in: [2, 1, { 1: 1 }] } };
   var expRecs5 = [{ a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: { 0: 1, 1: 2, 2: 3 } },
   { a: { 0: 2, 1: 1, 2: 3 } }];
   checkResult( dbcl, findCondition5, hintConf1, sortConf, expRecs5 );

   var expRecs5 = [{ a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: { 0: 1, 1: 2, 2: 3 } },
   { a: { 0: 2, 1: 1, 2: 3 } }];
   checkResult( dbcl, findCondition5, hintConf2, sortConf, expRecs5 );

   //all
   var findCondition5 = { "a.1": { $all: [2] } };
   var expRecs5 = [{ a: [1, 2, 3] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: { 0: 1, 1: 2, 2: 3 } }];
   checkResult( dbcl, findCondition5, hintConf1, sortConf, expRecs5 );

   var expRecs5 = [{ a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: { 0: 1, 1: 2, 2: 3 } }];
   checkResult( dbcl, findCondition5, hintConf2, sortConf, expRecs5 );

   //"a.$1" query
   //gt
   var findCondition1 = { "a.$1": { $gt: 2 } };
   var expRecs1 = [{ a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [3, 4, 5] },
   { a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$gte:2
   var findCondition2 = { "a.$1": { $gte: 2 } };
   var expRecs2 = [{ a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [3, 4, 5] },
   { a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   //$lt:2
   var findCondition3 = { "a.$1": { $lt: 2 } };
   var expRecs3 = [{ a: [-1, -2, -3] },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] }];
   checkResult( dbcl, findCondition3, hintConf, sortConf, expRecs3 );

   //$lte:2
   var findCondition4 = { "a.$1": { $lte: 2 } };
   var expRecs4 = [{ a: [-1, -2, -3] },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] }];
   checkResult( dbcl, findCondition4, hintConf, sortConf, expRecs4 );

   //mod
   var findCondition5 = { "a.$1": { $mod: [2, 1] } };
   var expRecs5 = [{ a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [3, 4, 5] }];
   checkResult( dbcl, findCondition5, hintConf, sortConf, expRecs5 );

   //et
   var findCondition6 = { "a.$1": { $et: 2 } };
   var expRecs6 = [{ a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] }];
   checkResult( dbcl, findCondition6, hintConf, sortConf, expRecs6 );

   //ne
   var findCondition7 = { "a.$1": { $ne: 2 } };
   var expRecs7 = [{ a: [-1, -2, -3] },
   { a: [3, 4, 5] },
   { a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { a: { 0: -1, 1: -2, 2: -3 } },
   { a: { 0: 1, 1: 2, 2: 3 } },
   { a: { 0: 2, 1: 1, 2: 3 } },
   { a: { 0: 2, 1: 3, 2: 4 } },
   { a: { 0: 3, 1: 4, 2: 5 } }];
   checkResult( dbcl, findCondition7, hintConf, sortConf, expRecs7 );

   //in
   var findCondition8 = { "a.$1": { $in: [2, 1, { 1: 1 }] } };
   var expRecs8 = [{ a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] }];
   checkResult( dbcl, findCondition8, hintConf, sortConf, expRecs8 );

   //all
   var findCondition9 = { "a.$1": { $all: [2] } };
   var expRecs9 = [{ a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] }];
   checkResult( dbcl, findCondition9, hintConf, sortConf, expRecs9 );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}
