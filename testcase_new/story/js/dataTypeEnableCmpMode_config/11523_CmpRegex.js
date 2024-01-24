/************************************
*@Description: set enablemixcmp=false;compare value set regex,
               indexScan and tableScan have the same result;
*@author:      zhaoyu
*@createdate:  2017.5.19
*@testlinkCase: seqDB-11523
**************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_11523";
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
   { a: { $options: "ih", $regex: "ab" } },
   { a: { $regex: "ab", $options: "ih" } },
   { a: { $regex: "ab", $options: "ij" } },
   { a: { $regex: "ab", $options: "j" } },
   { a: { $regex: "a" } },
   { a: { $regex: "b" } },
   { a: { b: 1 } },
   { a: [1, 20, 30] },
   { a: null },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { b: 1 }];
   dbcl.insert( doc );

   //gt
   var findConf1 = { a: { $gt: { $regex: "ab" } } };
   var expRecs1 = [{ a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "ab", $options: "ih" } },
   { a: { $regex: "ab", $options: "ij" } },
   { a: { $regex: "ab", $options: "j" } },
   { a: { $regex: "b" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   var findConf2 = { a: { $gt: { $regex: "ab", $options: "ij" } } };
   var expRecs2 = [{ a: { $regex: "ab", $options: "j" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   //gte
   var findConf3 = { a: { $gte: { $regex: "a" } } };
   var expRecs3 = [{ a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "ab", $options: "ih" } },
   { a: { $regex: "ab", $options: "ij" } },
   { a: { $regex: "ab", $options: "j" } },
   { a: { $regex: "a" } },
   { a: { $regex: "b" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   var findConf4 = { a: { $gte: { $regex: "ab", $options: "ij" } } };
   var expRecs4 = [{ a: { $regex: "ab", $options: "ij" } },
   { a: { $regex: "ab", $options: "j" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf4, hintConf, sortConf, expRecs4 );

   //lt
   var findConf5 = { a: { $lt: { $regex: "ab" } } };
   var expRecs5 = [{ a: 100 },
   { a: 200 },
   { a: 100.23 },
   { a: { $decimal: "300" } },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { $options: "ih", $regex: "ab" } },
   { a: { $regex: "a" } },
   { a: { b: 1 } },
   { a: [1, 20, 30] },
   { a: null },
   { a: { $minKey: 1 } }];
   checkResult( dbcl, findConf5, hintConf, sortConf, expRecs5 );

   var findConf6 = { a: { $lt: { $regex: "ab", $options: "ij" } } };
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
   { a: { $options: "ih", $regex: "ab" } },
   { a: { $regex: "ab", $options: "ih" } },
   { a: { $regex: "a" } },
   { a: { $regex: "b" } },
   { a: { b: 1 } },
   { a: [1, 20, 30] },
   { a: null },
   { a: { $minKey: 1 } }];
   checkResult( dbcl, findConf6, hintConf, sortConf, expRecs6 );

   //lte
   var findConf7 = { a: { $lte: { $regex: "a" } } };
   var expRecs7 = [{ a: 100 },
   { a: 200 },
   { a: 100.23 },
   { a: { $decimal: "300" } },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { $options: "ih", $regex: "ab" } },
   { a: { $regex: "a" } },
   { a: { b: 1 } },
   { a: [1, 20, 30] },
   { a: null },
   { a: { $minKey: 1 } }];
   checkResult( dbcl, findConf7, hintConf, sortConf, expRecs7 );

   var findConf8 = { a: { $lte: { $regex: "ab", $options: "ij" } } };
   var expRecs8 = [{ a: 100 },
   { a: 200 },
   { a: 100.23 },
   { a: { $decimal: "300" } },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $options: "ih", $regex: "ab" } },
   { a: { $regex: "ab", $options: "ih" } },
   { a: { $regex: "ab", $options: "ij" } },
   { a: { $regex: "a" } },
   { a: { $regex: "b" } },
   { a: { b: 1 } },
   { a: [1, 20, 30] },
   { a: null },
   { a: { $minKey: 1 } }];
   checkResult( dbcl, findConf8, hintConf, sortConf, expRecs8 );

   //et
   var findConf9 = { a: { $et: { $regex: "a" } } };
   var expRecs9 = [{ a: { $regex: "a" } }];
   checkResult( dbcl, findConf9, hintConf, sortConf, expRecs9 );

   var findConf10 = { a: { $et: { $regex: "ab", $options: "ij" } } };
   var expRecs10 = [{ a: { $regex: "ab", $options: "ij" } }];
   checkResult( dbcl, findConf10, hintConf, sortConf, expRecs10 );

   //ne
   var findConf11 = { a: { $ne: { $regex: "a" } } };
   var expRecs11 = [{ a: 100 },
   { a: 200 },
   { a: 100.23 },
   { a: { $decimal: "300" } },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $options: "ih", $regex: "ab" } },
   { a: { $regex: "ab", $options: "ih" } },
   { a: { $regex: "ab", $options: "ij" } },
   { a: { $regex: "ab", $options: "j" } },
   { a: { $regex: "b" } },
   { a: { b: 1 } },
   { a: [1, 20, 30] },
   { a: null },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf11, hintConf, sortConf, expRecs11 );

   var findConf12 = { a: { $ne: { $regex: "ab", $options: "ij" } } };
   var expRecs12 = [{ a: 100 },
   { a: 200 },
   { a: 100.23 },
   { a: { $decimal: "300" } },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $options: "ih", $regex: "ab" } },
   { a: { $regex: "ab", $options: "ih" } },
   { a: { $regex: "ab", $options: "j" } },
   { a: { $regex: "a" } },
   { a: { $regex: "b" } },
   { a: { b: 1 } },
   { a: [1, 20, 30] },
   { a: null },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf12, hintConf, sortConf, expRecs12 );
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}
