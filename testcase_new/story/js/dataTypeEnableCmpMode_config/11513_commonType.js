/************************************
*@Description: set enablemixcmp=false;compare value set common data type,
               indexScan and tableScan have the same result;
*@author:      zhaoyu
*@createdate:  2017.5.19
*@testlinkCase: seqDB-11513
**************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_11513";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //create index
   commCreateIndex( dbcl, "a", { a: 1 } );

   var hintConf = [{ "": "a" }, { "": null }];
   var sortConf = { _id: 1 };

   //insert all type data 
   var doc = [{ a: { $minKey: 1 } },
   { b: 1 },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   dbcl.insert( doc );

   //gt
   var findConf1 = { a: { $gt: null } };
   var expRecs1 = [{ a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   var findConf2 = { a: { $gt: 22 } };
   var expRecs2 = [{ a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   var findConf3 = { a: { $gt: "ab" } };
   var expRecs3 = [{ a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   var findConf4 = { a: { $gt: { d: 2 } } };
   var expRecs4 = [{ a: { f: 0 } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf4, hintConf, sortConf, expRecs4 );

   var findConf5 = { a: { $gt: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } } };
   var expRecs5 = [{ a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf5, hintConf, sortConf, expRecs5 );

   var findConf6 = { a: { $gt: { $oid: "591e6a7e3fb026ec2f000008" } } };
   var expRecs6 = [{ a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf6, hintConf, sortConf, expRecs6 );

   var findConf7 = { a: { $gt: false } };
   var expRecs7 = [{ a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf7, hintConf, sortConf, expRecs7 );

   var findConf8 = { a: { $gt: { $date: "2014-01-01" } } };
   var expRecs8 = [{ a: { $date: "2014-01-03" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf8, hintConf, sortConf, expRecs8 );

   var findConf1 = { a: { $gt: 22, $lt: { $timestamp: "2013-06-05-16.10.33.000000" } } };
   var expRecs1 = [{ a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   //gte
   var findConf1 = { a: { $gte: null } };
   var expRecs1 = [{ a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   var findConf2 = { a: { $gte: 22 } };
   var expRecs2 = [{ a: 22 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   var findConf3 = { a: { $gte: "ab" } };
   var expRecs3 = [{ a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   var findConf4 = { a: { $gte: { d: 2 } } };
   var expRecs4 = [{ a: { d: 2 } }, { a: { f: 0 } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf4, hintConf, sortConf, expRecs4 );

   var findConf5 = { a: { $gte: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } } };
   var expRecs5 = [{ a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf5, hintConf, sortConf, expRecs5 );

   var findConf6 = { a: { $gte: { $oid: "591e6a7e3fb026ec2f000008" } } };
   var expRecs6 = [{ a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf6, hintConf, sortConf, expRecs6 );

   var findConf7 = { a: { $gte: false } };
   var expRecs7 = [{ a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf7, hintConf, sortConf, expRecs7 );

   var findConf6 = { a: { $gte: { $date: "2014-01-01" } } };
   var expRecs6 = [{ a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf6, hintConf, sortConf, expRecs6 );

   //lt
   var findConf1 = { a: { $lt: { $date: "2014-01-01" } } };
   var expRecs1 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   var findConf2 = { a: { $lt: true } };
   var expRecs2 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: false }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   var findConf3 = { a: { $lt: { $oid: "591e6a7e3fb026ec2f000008" } } };
   var expRecs3 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } }];
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   var findConf4 = { a: { $lt: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } } };
   var expRecs4 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] }];
   checkResult( dbcl, findConf4, hintConf, sortConf, expRecs4 );

   var findConf5 = { a: { $lt: { d: 2 } } };
   var expRecs5 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] }];
   checkResult( dbcl, findConf5, hintConf, sortConf, expRecs5 );

   var findConf6 = { a: { $lt: "ab" } };
   var expRecs6 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" },
   { a: [1, 2, 3] }, { a: [23, 4, 26] }];
   checkResult( dbcl, findConf6, hintConf, sortConf, expRecs6 );

   var findConf7 = { a: { $lt: 22 } };
   var expRecs7 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 3 },
   { a: [1, 2, 3] }, { a: [23, 4, 26] }];
   checkResult( dbcl, findConf7, hintConf, sortConf, expRecs7 );

   var findConf8 = { a: { $lt: null } };
   var expRecs8 = [{ a: { $minKey: 1 } }];
   checkResult( dbcl, findConf8, hintConf, sortConf, expRecs8 );

   //lte
   var findConf1 = { a: { $lte: { $date: "2014-01-01" } } };
   var expRecs1 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   var findConf2 = { a: { $lte: true } };
   var expRecs2 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   var findConf3 = { a: { $lte: { $oid: "591e6a7e3fb026ec2f000008" } } };
   var expRecs3 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } }];
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   var findConf4 = { a: { $lte: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } } };
   var expRecs4 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } }];
   checkResult( dbcl, findConf4, hintConf, sortConf, expRecs4 );

   var findConf5 = { a: { $lte: { d: 2 } } };
   var expRecs5 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] }];
   checkResult( dbcl, findConf5, hintConf, sortConf, expRecs5 );

   var findConf6 = { a: { $lte: "ab" } };
   var expRecs6 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" },
   { a: [1, 2, 3] }, { a: [23, 4, 26] }];
   checkResult( dbcl, findConf6, hintConf, sortConf, expRecs6 );

   var findConf7 = { a: { $lte: 22 } };
   var expRecs7 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 },
   { a: [1, 2, 3] }, { a: [23, 4, 26] }];
   checkResult( dbcl, findConf7, hintConf, sortConf, expRecs7 );

   var findConf8 = { a: { $lte: null } };
   var expRecs8 = [{ a: { $minKey: 1 } },
   { a: null }];
   checkResult( dbcl, findConf8, hintConf, sortConf, expRecs8 );

   //et
   var findConf1 = { a: { $et: { $date: "2014-01-01" } } };
   var expRecs1 = [{ a: { $date: "2014-01-01" } }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   var findConf2 = { a: { $et: true } };
   var expRecs2 = [{ a: true }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   var findConf3 = { a: { $et: { $oid: "591e6a7e3fb026ec2f000008" } } };
   var expRecs3 = [{ a: { $oid: "591e6a7e3fb026ec2f000008" } }];
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   var findConf4 = { a: { $et: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } } };
   var expRecs4 = [{ a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } }];
   checkResult( dbcl, findConf4, hintConf, sortConf, expRecs4 );

   var findConf5 = { a: { $et: { d: 2 } } };
   var expRecs5 = [{ a: { d: 2 } }];
   checkResult( dbcl, findConf5, hintConf, sortConf, expRecs5 );

   var findConf6 = { a: { $et: "ab" } };
   var expRecs6 = [{ a: "ab" }];
   checkResult( dbcl, findConf6, hintConf, sortConf, expRecs6 );

   var findConf7 = { a: { $et: 3 } };
   var expRecs7 = [{ a: 3 }, { a: [1, 2, 3] }];
   checkResult( dbcl, findConf7, hintConf, sortConf, expRecs7 );

   var findConf8 = { a: { $et: null } };
   var expRecs8 = [{ a: null }];
   checkResult( dbcl, findConf8, hintConf, sortConf, expRecs8 );

   //ne
   var findConf1 = { a: { $ne: null } };
   var expRecs1 = [{ a: { $minKey: 1 } },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   var findConf2 = { a: { $ne: 3 } };
   var expRecs2 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   var findConf3 = { a: { $ne: "ab" } };
   var expRecs3 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   var findConf4 = { a: { $ne: { d: 2 } } };
   var expRecs4 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { f: 0 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf4, hintConf, sortConf, expRecs4 );

   var findConf5 = { a: { $ne: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } } };
   var expRecs5 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf5, hintConf, sortConf, expRecs5 );

   var findConf6 = { a: { $ne: { $oid: "591e6a7e3fb026ec2f000013" } } };
   var expRecs6 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf6, hintConf, sortConf, expRecs6 );

   var findConf7 = { a: { $ne: false } };
   var expRecs7 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf7, hintConf, sortConf, expRecs7 );

   var findConf8 = { a: { $ne: { $date: "2014-01-03" } } };
   var expRecs8 = [{ a: { $minKey: 1 } },
   { a: null },
   { a: 22 }, { a: 3 }, { a: 24 },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { b: 1 } }, { a: { d: 2 } }, { a: { f: 0 } },
   { a: [1, 2, 3] }, { a: [23, 4, 26] },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: { $oid: "591e6a7e3fb026ec2f000013" } },
   { a: { $oid: "591e6a7e3fb026ec2f000008" } },
   { a: true }, { a: false },
   { a: { $date: "2014-01-01" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf8, hintConf, sortConf, expRecs8 );

   //mod
   var findConf1 = { a: { $mod: [2, 1] } };
   var expRecs1 = [{ a: 3 },
   { a: [1, 2, 3] }, { a: [23, 4, 26] }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   //{isnull:1}
   var findConf2 = { a: { $isnull: 1 } };
   var expRecs2 = [{ b: 1 },
   { a: null }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   //{exists:0}
   var findConf3 = { a: { $exists: 0 } };
   var expRecs3 = [{ b: 1 }];
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   //field
   var findConf4 = { a: { $field: "b" } };
   checkResult( dbcl, findConf4, hintConf, sortConf, [] );

   var findConf5 = { b: { $field: "a" } };
   checkResult( dbcl, findConf5, hintConf, sortConf, [] );

   var findConf6 = { c: { $field: "d" } };
   checkResult( dbcl, findConf6, hintConf, sortConf, [] );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}
