/************************************
*@Description: set enablemixcmp=false;compare value set minKey or maxKey,
               indexScan and tableScan have the same result;
*@author:      zhaoyu
*@createdate:  2017.5.19
*@testlinkCase: seqDB-11519
**************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_11519";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //create index
   commCreateIndex( dbcl, "a", { a: 1 } );

   var hintConf = [{ "": "a" }, { "": null }];
   var sortConf = { _id: 1 };

   //insert all type data 
   var doc = [{ a: 1 },
   { a: { $numberLong: "2" } },
   { a: 1.23 },
   { a: { $decimal: "3" } },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } }, { a: { $date: "2014-01-02" } }, { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $timestamp: "2015-06-05-16.10.34.000000" } },
   { a: { $timestamp: "2015-06-05-16.10.35.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { b: 1 } },
   { a: [1, 2, 3] },
   { a: null },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { b: 1 }];
   dbcl.insert( doc );

   //minKey and maxKey
   var findConf1 = { a: { $gt: { $minKey: 1 }, $lt: { $maxKey: 1 } } };
   var expRecs1 = [{ a: 1 },
   { a: 2 },
   { a: 1.23 },
   { a: { $decimal: "3" } },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } }, { a: { $date: "2014-01-02" } }, { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $timestamp: "2015-06-05-16.10.34.000000" } },
   { a: { $timestamp: "2015-06-05-16.10.35.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { b: 1 } },
   { a: [1, 2, 3] },
   { a: null }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   var findConf2 = { a: { $gte: { $minKey: 1 }, $lte: { $maxKey: 1 } } };
   var expRecs2 = [{ a: 1 },
   { a: 2 },
   { a: 1.23 },
   { a: { $decimal: "3" } },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } }, { a: { $date: "2014-01-02" } }, { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $timestamp: "2015-06-05-16.10.34.000000" } },
   { a: { $timestamp: "2015-06-05-16.10.35.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { b: 1 } },
   { a: [1, 2, 3] },
   { a: null },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   //maxKey or minKey
   var findConf3 = { $or: [{ a: { $gt: { $maxKey: 1 } } }, { a: { $lt: { $minKey: 1 } } }] };
   var expRecs3 = [];
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   var findConf4 = { $or: [{ a: { $gte: { $maxKey: 1 } } }, { a: { $lte: { $minKey: 1 } } }] };
   var expRecs4 = [{ a: { $minKey: 1 } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf4, hintConf, sortConf, expRecs4 );

   //int and minKey
   var findConf5 = { a: { $gte: { $minKey: 1 }, $lte: 4 } };
   var expRecs5 = [{ a: 1 },
   { a: 2 },
   { a: 1.23 },
   { a: { $decimal: "3" } },
   { a: [1, 2, 3] },
   { a: null },
   { a: { $minKey: 1 } }];
   checkResult( dbcl, findConf5, hintConf, sortConf, expRecs5 );

   //string and maxKey,SEQUOIADBMAINSTREAM-2450
   var findConf6 = { a: { $gte: "ab", $lte: { $maxKey: 1 } } };
   var expRecs6 = [{ a: "ab" }, { a: "b" },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } }, { a: { $date: "2014-01-02" } }, { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $timestamp: "2015-06-05-16.10.34.000000" } },
   { a: { $timestamp: "2015-06-05-16.10.35.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { b: 1 } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf6, hintConf, sortConf, expRecs6 );

   //date and minKey
   var findConf7 = { a: { $gt: { $minKey: 1 }, $lt: { $date: "2014-01-02" } } };
   var expRecs7 = [{ a: 1 },
   { a: 2 },
   { a: 1.23 },
   { a: { $decimal: "3" } },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { b: 1 } },
   { a: [1, 2, 3] },
   { a: null }];
   checkResult( dbcl, findConf7, hintConf, sortConf, expRecs7 );

   //timestamp and maxKey
   var findConf8 = { a: { $gt: { $timestamp: "2015-06-05-16.10.34.000000" }, $lt: { $maxKey: 1 } } };
   var expRecs8 = [{ a: { $timestamp: "2015-06-05-16.10.35.000000" } },
   { a: { $regex: "^a", $options: "i" } }];
   checkResult( dbcl, findConf8, hintConf, sortConf, expRecs8 );

   //bool and minKey
   var findConf9 = { a: { $gt: false, $lt: { $maxKey: 1 } } };
   var expRecs9 = [{ a: true },
   { a: { $date: "2014-01-01" } }, { a: { $date: "2014-01-02" } }, { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $timestamp: "2015-06-05-16.10.34.000000" } },
   { a: { $timestamp: "2015-06-05-16.10.35.000000" } },
   { a: { $regex: "^a", $options: "i" } }];
   checkResult( dbcl, findConf9, hintConf, sortConf, expRecs9 );

   //et
   var findConf10 = { a: { $maxKey: 1 } };
   var expRecs10 = [{ a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf10, hintConf, sortConf, expRecs10 );

   var findConf11 = { a: { $minKey: 1 } };
   var expRecs11 = [{ a: { $minKey: 1 } }];
   checkResult( dbcl, findConf11, hintConf, sortConf, expRecs11 );

   //ne
   var findConf12 = { a: { $ne: { $maxKey: 1 } } };
   var expRecs12 = [{ a: 1 },
   { a: 2 },
   { a: 1.23 },
   { a: { $decimal: "3" } },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } }, { a: { $date: "2014-01-02" } }, { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $timestamp: "2015-06-05-16.10.34.000000" } },
   { a: { $timestamp: "2015-06-05-16.10.35.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { b: 1 } },
   { a: [1, 2, 3] },
   { a: null },
   { a: { $minKey: 1 } }];
   checkResult( dbcl, findConf12, hintConf, sortConf, expRecs12 );

   var findConf13 = { a: { $ne: { $minKey: 1 } } };
   var expRecs13 = [{ a: 1 },
   { a: 2 },
   { a: 1.23 },
   { a: { $decimal: "3" } },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } }, { a: { $date: "2014-01-02" } }, { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $timestamp: "2015-06-05-16.10.34.000000" } },
   { a: { $timestamp: "2015-06-05-16.10.35.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { b: 1 } },
   { a: [1, 2, 3] },
   { a: null },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf13, hintConf, sortConf, expRecs13 );

   var findConf14 = { a: { $gt: { $minKey: 1 } } };
   var expRecs14 = [{ a: 1 },
   { a: 2 },
   { a: 1.23 },
   { a: { $decimal: "3" } },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } }, { a: { $date: "2014-01-02" } }, { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $timestamp: "2015-06-05-16.10.34.000000" } },
   { a: { $timestamp: "2015-06-05-16.10.35.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { b: 1 } },
   { a: [1, 2, 3] },
   { a: null },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findConf14, hintConf, sortConf, expRecs14 );

   var findConf15 = { a: { $lt: { $maxKey: 1 } } };
   var expRecs15 = [{ a: 1 },
   { a: 2 },
   { a: 1.23 },
   { a: { $decimal: "3" } },
   { a: "aa" }, { a: "ab" }, { a: "b" },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } }, { a: { $date: "2014-01-02" } }, { a: { $date: "2014-01-03" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $timestamp: "2015-06-05-16.10.34.000000" } },
   { a: { $timestamp: "2015-06-05-16.10.35.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^a", $options: "i" } },
   { a: { b: 1 } },
   { a: [1, 2, 3] },
   { a: null },
   { a: { $minKey: 1 } }];
   checkResult( dbcl, findConf15, hintConf, sortConf, expRecs15 );
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}
