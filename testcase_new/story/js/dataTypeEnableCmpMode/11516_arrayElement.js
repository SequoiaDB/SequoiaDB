/************************************
*@Description: set enablemixcmp = false; compare value set array, 
indexScan and tableScan have the same result; 
*@author:      zhaoyu
*@createdate:  2017.5.18
*@testlinkCase: seqDB-11520
**************************************/
main( test );
function test ()
{
   var clName = COMMCLNAME + "_11520";
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
   { a: { $maxKey: 1 } }];;
   dbcl.insert( doc );

   //a query
   //gt
   var findConf1 = { a: { $gt: 2 } };
   var expRecs1 = [{ a: 3 }, { a: 4 },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [3, 4, 5] }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   //gte
   var findConf2 = { a: { $gte: 2 } };
   var expRecs2 = [{ a: 2 }, { a: 3 }, { a: 4 },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [3, 4, 5] }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   //lte
   var findConf3 = { a: { $lte: 2 } };
   var expRecs3 = [{ a: 2 },
   { a: [-1, -2, -3] },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] }];
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   //lt
   var findConf4 = { a: { $lt: 2 } };
   var expRecs4 = [{ a: [-1, -2, -3] },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] }];
   checkResult( dbcl, findConf4, hintConf, sortConf, expRecs4 );

   //et
   var findConf5 = { a: { $et: 2 } };
   var expRecs5 = [{ a: 2 },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] }];
   checkResult( dbcl, findConf5, hintConf, sortConf, expRecs5 );

   //ne
   var findConf6 = { a: { $ne: 2 } };
   var expRecs6 = [{ a: { $minKey: 1 } },
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
   checkResult( dbcl, findConf6, hintConf, sortConf, expRecs6 );

   //mod
   var findConf7 = { a: { $mod: [2, 1] } };
   var expRecs7 = [{ a: 3 },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [3, 4, 5] }];
   checkResult( dbcl, findConf7, hintConf, sortConf, expRecs7 );

   //in
   var findConf8 = { a: { $in: [1, 2, 3] } };
   var expRecs8 = [{ a: 2 }, { a: 3 },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [3, 4, 5] }];
   checkResult( dbcl, findConf8, hintConf, sortConf, expRecs8 );

   //all
   var findConf9 = { a: { $all: [1, 2, 3] } };
   var expRecs9 = [{ a: [1, 2, 3] },
   { a: [2, 1, 3] }];
   checkResult( dbcl, findConf9, hintConf, sortConf, expRecs9 );

   //"a.1" query, index scan use {"a.1":1} or {"a":1} has different result; 
   var hintConf1 = [{ "": "a" }, { "": null }];
   var hintConf2 = [{ "": "a1" }];

   //gt
   var findConf10 = { "a.1": { $gt: 2 } };
   var expRecs10 = [{ a: [2, 3, 4] },
   { a: [3, 4, 5] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { a: { 0: 2, 1: 3, 2: 4 } },
   { a: { 0: 3, 1: 4, 2: 5 } }];
   checkResult( dbcl, findConf10, hintConf1, sortConf, expRecs10 );

   var expRecs10 = [{ a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { a: { 0: 2, 1: 3, 2: 4 } },
   { a: { 0: 3, 1: 4, 2: 5 } }];
   checkResult( dbcl, findConf10, hintConf2, sortConf, expRecs10 );

   //gte
   var findConf11 = { "a.1": { $gte: 2 } };
   var expRecs11 = [{ a: [1, 2, 3] },
   { a: [2, 3, 4] },
   { a: [3, 4, 5] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { a: { 0: 1, 1: 2, 2: 3 } },
   { a: { 0: 2, 1: 3, 2: 4 } },
   { a: { 0: 3, 1: 4, 2: 5 } }];
   checkResult( dbcl, findConf11, hintConf1, sortConf, expRecs11 );

   var expRecs11 = [{ a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { a: { 0: 1, 1: 2, 2: 3 } },
   { a: { 0: 2, 1: 3, 2: 4 } },
   { a: { 0: 3, 1: 4, 2: 5 } }];
   checkResult( dbcl, findConf11, hintConf2, sortConf, expRecs11 );

   //lte
   var findConf12 = { "a.1": { $lte: 2 } };
   var expRecs12 = [{ a: [-1, -2, -3] },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: { 0: -1, 1: -2, 2: -3 } },
   { a: { 0: 1, 1: 2, 2: 3 } },
   { a: { 0: 2, 1: 1, 2: 3 } }];
   checkResult( dbcl, findConf12, hintConf1, sortConf, expRecs12 );

   var expRecs12 = [{ a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: { 0: -1, 1: -2, 2: -3 } },
   { a: { 0: 1, 1: 2, 2: 3 } },
   { a: { 0: 2, 1: 1, 2: 3 } }];
   checkResult( dbcl, findConf12, hintConf2, sortConf, expRecs12 );

   //lt
   var findConf13 = { "a.1": { $lt: 2 } };
   var expRecs13 = [{ a: [-1, -2, -3] },
   { a: [2, 1, 3] },
   { a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: { 0: -1, 1: -2, 2: -3 } },
   { a: { 0: 2, 1: 1, 2: 3 } }];
   checkResult( dbcl, findConf13, hintConf1, sortConf, expRecs13 );

   var expRecs13 = [{ a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: { 0: -1, 1: -2, 2: -3 } },
   { a: { 0: 2, 1: 1, 2: 3 } }];
   checkResult( dbcl, findConf13, hintConf2, sortConf, expRecs13 );

   //mod
   var findConf14 = { "a.1": { $mod: [2, 1] } };
   var expRecs14 = [{ a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: { 0: 2, 1: 1, 2: 3 } },
   { a: { 0: 2, 1: 3, 2: 4 } }];
   checkResult( dbcl, findConf14, hintConf1, sortConf, expRecs14 );

   var expRecs14 = [{ a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: { 0: 2, 1: 1, 2: 3 } },
   { a: { 0: 2, 1: 3, 2: 4 } }];
   checkResult( dbcl, findConf14, hintConf2, sortConf, expRecs14 );

   //ne, SEQUOIADBMAINSTREAM-2469
   var findConf15 = { "a.1": { $ne: 2 } };
   var expRecs15 = [{ a: [-1, -2, -3] },
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
   checkResult( dbcl, findConf15, hintConf1, sortConf, expRecs15 );

   var expRecs15 = [{ a: [{ 0: -1 }, { 1: -2 }, { 2: -3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: [{ 0: 3 }, { 1: 4 }, { 2: 5 }] },
   { a: { 0: -1, 1: -2, 2: -3 } },
   { a: { 0: 2, 1: 1, 2: 3 } },
   { a: { 0: 2, 1: 3, 2: 4 } },
   { a: { 0: 3, 1: 4, 2: 5 } }];
   checkResult( dbcl, findConf15, hintConf2, sortConf, expRecs15 );

   //et
   var findConf16 = { "a.1": { $et: 2 } };
   var expRecs16 = [{ a: [1, 2, 3] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: { 0: 1, 1: 2, 2: 3 } }];
   checkResult( dbcl, findConf16, hintConf1, sortConf, expRecs16 );

   var expRecs17 = [{ a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: { 0: 1, 1: 2, 2: 3 } }];
   checkResult( dbcl, findConf16, hintConf2, sortConf, expRecs17 );

   //in
   var findConf18 = { "a.1": { $in: [1, 2, 3] } };
   var expRecs18 = [{ a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: { 0: 1, 1: 2, 2: 3 } },
   { a: { 0: 2, 1: 1, 2: 3 } },
   { a: { 0: 2, 1: 3, 2: 4 } }];
   checkResult( dbcl, findConf18, hintConf1, sortConf, expRecs18 );

   var expRecs19 = [{ a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] },
   { a: [{ 0: 2 }, { 1: 3 }, { 2: 4 }] },
   { a: { 0: 1, 1: 2, 2: 3 } },
   { a: { 0: 2, 1: 1, 2: 3 } },
   { a: { 0: 2, 1: 3, 2: 4 } }];
   checkResult( dbcl, findConf18, hintConf2, sortConf, expRecs19 );

   //all
   var findConf20 = { "a.1": { $all: [2] } };
   var expRecs20 = [{ a: [1, 2, 3] },
   { a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: { 0: 1, 1: 2, 2: 3 } }];
   checkResult( dbcl, findConf20, hintConf1, sortConf, expRecs20 );

   var expRecs21 = [{ a: [{ 0: 1 }, { 1: 2 }, { 2: 3 }] },
   { a: { 0: 1, 1: 2, 2: 3 } }];
   checkResult( dbcl, findConf20, hintConf2, sortConf, expRecs21 );

   //"a.$1" query, 
   //gt
   var findConf22 = { "a.$1": { $gt: 2 } };
   var expRecs22 = [{ a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [3, 4, 5] }];
   checkResult( dbcl, findConf22, hintConf, sortConf, expRecs22 );

   //gte
   var findConf22 = { "a.$1": { $gte: 2 } };
   var expRecs22 = [{ a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [3, 4, 5] }];
   checkResult( dbcl, findConf22, hintConf, sortConf, expRecs22 );

   //lte
   var findConf22 = { "a.$1": { $lte: 2 } };
   var expRecs22 = [{ a: [-1, -2, -3] },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] }];
   checkResult( dbcl, findConf22, hintConf, sortConf, expRecs22 );

   //lt
   var findConf22 = { "a.$1": { $lt: 2 } };
   var expRecs22 = [{ a: [-1, -2, -3] },
   { a: [1, 2, 3] },
   { a: [2, 1, 3] }];
   checkResult( dbcl, findConf22, hintConf, sortConf, expRecs22 );

   //et
   var findConf22 = { "a.$1": { $et: 2 } };
   var expRecs22 = [{ a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] }];
   checkResult( dbcl, findConf22, hintConf, sortConf, expRecs22 );

   //ne
   var findConf22 = { "a.$1": { $ne: 2 } };
   var expRecs22 = [{ a: [-1, -2, -3] },
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
   checkResult( dbcl, findConf22, hintConf, sortConf, expRecs22 );

   //mod
   var findConf22 = { "a.$1": { $mod: [2, 1] } };
   var expRecs22 = [{ a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [3, 4, 5] }];
   checkResult( dbcl, findConf22, hintConf, sortConf, expRecs22 );

   //in
   var findConf22 = { "a.$1": { $in: [2, { 1: 1 }, 3] } };
   var expRecs22 = [{ a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] },
   { a: [3, 4, 5] },
   { a: [{ 0: 2 }, { 1: 1 }, { 2: 3 }] }];
   checkResult( dbcl, findConf22, hintConf, sortConf, expRecs22 );

   //all
   var findConf22 = { "a.$1": { $all: [2] } };
   var expRecs22 = [{ a: [1, 2, 3] },
   { a: [2, 1, 3] },
   { a: [2, 3, 4] }];
   checkResult( dbcl, findConf22, hintConf, sortConf, expRecs22 );
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}