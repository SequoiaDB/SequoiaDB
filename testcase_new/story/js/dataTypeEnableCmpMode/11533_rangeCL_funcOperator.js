/************************************
*@Description: match range sharding, use type/size, gt/gte/lt/lte/et/ne/mod/in/all/{isnull:1}/{exists:0}
data type: int/numberLong/double/decimal/string/bool/date/timestamp/binary/regex/json/array/null/minKey/maxKey
*@author:      liuxiaoxuan
*@createdate:  2017.5.20
*@testlinkCase: seqDB-11533
**************************************/

main( test );
function test ()
{
   var clName = COMMCLNAME + "_11533";

   //clean environment before test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //check test environment before split

   //standalone can not split
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //less two groups, can not split
   var allGroupName = getGroupName( db );
   if( 1 === allGroupName.length )
   {
      return;
   }

   var ClOption = { ShardingKey: { "a": 1 }, ShardingType: "range" };
   var dbcl = commCreateCL( db, COMMCSNAME, clName, ClOption, true, true );

   var hintConf = [{ "": null }, { "": "$shard" }];
   var sortConf = { _id: 1 };

   //insert data
   var doc = [{ a: 10 },
   { a: 100 },
   { a: 1001.02 },
   { a: { $decimal: "20170519.09" } },
   { a: { $numberLong: "1000001" } },
   { a: { $date: "2017-05-19" } },
   { a: { $timestamp: "2017-05-19-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "abc" },
   { a: MinKey() },
   { a: MaxKey() },
   { a: true },
   { a: false },
   { a: { name: "Jack" } },
   { a: [10] },
   { a: [102.03] },
   { a: [1001] },
   { a: ["a"] },
   { a: ["z"] },
   { b: 1 }];
   dbcl.insert( doc );

   //split cl
   var startCondition1 = 50;
   ClSplitOneTimes( COMMCSNAME, clName, startCondition1, null );

   //$size + $et
   var findCondition1 = { a: { $size: 1, $et: 1 } };
   var expRecs1 = [{ a: { name: "Jack" } },
   { a: [10] },
   { a: [102.03] },
   { a: [1001] },
   { a: ["a"] },
   { a: ["z"] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$size + $gt
   var findCondition2 = { a: { $size: 1, $gt: 0 } };
   var expRecs2 = [{ a: { name: "Jack" } },
   { a: [10] },
   { a: [102.03] },
   { a: [1001] },
   { a: ["a"] },
   { a: ["z"] }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   //$size + $gte
   var findCondition3 = { a: { $size: 1, $gte: 1 } };
   var expRecs3 = [{ a: { name: "Jack" } },
   { a: [10] },
   { a: [102.03] },
   { a: [1001] },
   { a: ["a"] },
   { a: ["z"] }];
   checkResult( dbcl, findCondition3, hintConf, sortConf, expRecs3 );

   //$size + $lt
   var findCondition4 = { a: { $size: 1, $lt: 2 } };
   var expRecs4 = [{ a: { name: "Jack" } },
   { a: [10] },
   { a: [102.03] },
   { a: [1001] },
   { a: ["a"] },
   { a: ["z"] }];
   checkResult( dbcl, findCondition4, hintConf, sortConf, expRecs4 );

   //$size + $lte
   var findCondition5 = { a: { $size: 1, $lte: 1 } };
   var expRecs5 = [{ a: { name: "Jack" } },
   { a: [10] },
   { a: [102.03] },
   { a: [1001] },
   { a: ["a"] },
   { a: ["z"] }];
   checkResult( dbcl, findCondition5, hintConf, sortConf, expRecs5 );

   //$size + $ne
   var findCondition6 = { a: { $size: 1, $ne: 1 } };
   var expRecs6 = [{ a: 10 },
   { a: 100 },
   { a: 1001.02 },
   { a: { $decimal: "20170519.09" } },
   { a: 1000001 },
   { a: { $date: "2017-05-19" } },
   { a: { $timestamp: "2017-05-19-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "abc" },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: true },
   { a: false }];
   checkResult( dbcl, findCondition6, hintConf, sortConf, expRecs6 );

   //$size + $mod
   var findCondition7 = { a: { $size: 1, $mod: [2, 1] } };
   var expRecs7 = [{ a: { name: "Jack" } },
   { a: [10] },
   { a: [102.03] },
   { a: [1001] },
   { a: ["a"] },
   { a: ["z"] }];
   checkResult( dbcl, findCondition7, hintConf, sortConf, expRecs7 );

   //$size + $in
   var findCondition8 = { a: { $size: 1, $in: [1, 2] } };
   var expRecs8 = [{ a: { name: "Jack" } },
   { a: [10] },
   { a: [102.03] },
   { a: [1001] },
   { a: ["a"] },
   { a: ["z"] }];
   checkResult( dbcl, findCondition8, hintConf, sortConf, expRecs8 );

   //$size + $all
   var findCondition9 = { a: { $size: 1, $all: [1] } };
   var expRecs9 = [{ a: { name: "Jack" } },
   { a: [10] },
   { a: [102.03] },
   { a: [1001] },
   { a: ["a"] },
   { a: ["z"] }];
   checkResult( dbcl, findCondition9, hintConf, sortConf, expRecs9 );

   //$size + $isnull
   var findCondition10 = { a: { $size: 1, $isnull: 1 } };
   var expRecs10 = [{ a: 10 },
   { a: 100 },
   { a: 1001.02 },
   { a: { $decimal: "20170519.09" } },
   { a: 1000001 },
   { a: { $date: "2017-05-19" } },
   { a: { $timestamp: "2017-05-19-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "abc" },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: true },
   { a: false },
   { b: 1 }];
   checkResult( dbcl, findCondition10, hintConf, sortConf, expRecs10 );

   //$size + $exists
   var findCondition11 = { a: { $size: 1, $exists: 0 } };
   var expRecs11 = [{ b: 1 }];
   checkResult( dbcl, findCondition11, hintConf, sortConf, expRecs11 );

   //$type + $et
   var findCondition12 = { a: { $type: 1, $et: 4 } };
   var expRecs12 = [{ a: [10] },
   { a: [102.03] },
   { a: [1001] },
   { a: ["a"] },
   { a: ["z"] }];
   checkResult( dbcl, findCondition12, hintConf, sortConf, expRecs12 );

   //$type + $gt
   var findCondition13 = { a: { $type: 1, $gt: 45 } };
   var expRecs13 = [{ a: { $decimal: "20170519.09" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findCondition13, hintConf, sortConf, expRecs13 );

   //$type + $gte
   var findCondition14 = { a: { $type: 1, $gte: 100 } };
   var expRecs14 = [{ a: { $decimal: "20170519.09" } },
   { a: { $maxKey: 1 } }];
   checkResult( dbcl, findCondition14, hintConf, sortConf, expRecs14 );

   //$type + $lt
   var findCondition15 = { a: { $type: 1, $lt: 7 } };
   var expRecs15 = [{ a: 1001.02 },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: "abc" },
   { a: { $minKey: 1 } },
   { a: { name: "Jack" } },
   { a: [10] },
   { a: [102.03] },
   { a: [1001] },
   { a: ["a"] },
   { a: ["z"] }];
   checkResult( dbcl, findCondition15, hintConf, sortConf, expRecs15 );

   //$type + $lte
   var findCondition16 = { a: { $type: 1, $lte: 7 } };
   var expRecs16 = [{ a: 1001.02 },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "abc" },
   { a: { $minKey: 1 } },
   { a: { name: "Jack" } },
   { a: [10] },
   { a: [102.03] },
   { a: [1001] },
   { a: ["a"] },
   { a: ["z"] }];
   checkResult( dbcl, findCondition16, hintConf, sortConf, expRecs16 );

   //$type + $ne
   var findCondition17 = { a: { $type: 1, $ne: 1 } };
   var expRecs17 = [{ a: 10 },
   { a: 100 },
   { a: { $decimal: "20170519.09" } },
   { a: 1000001 },
   { a: { $date: "2017-05-19" } },
   { a: { $timestamp: "2017-05-19-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "abc" },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: true },
   { a: false },
   { a: { name: "Jack" } },
   { a: [10] },
   { a: [102.03] },
   { a: [1001] },
   { a: ["a"] },
   { a: ["z"] }];
   checkResult( dbcl, findCondition17, hintConf, sortConf, expRecs17 );

   //$type + $mod
   var findCondition18 = { a: { $type: 1, $mod: [2, 1] } };
   var expRecs18 = [{ a: 1001.02 },
   { a: { $date: "2017-05-19" } },
   { a: { $timestamp: "2017-05-19-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: { $maxKey: 1 } },
   { a: { name: "Jack" } }];
   checkResult( dbcl, findCondition18, hintConf, sortConf, expRecs18 );

   //$type + $in
   var findCondition19 = { a: { $type: 1, $in: [1, -10, 1000, 2] } };
   var expRecs19 = [{ a: 1001.02 },
   { a: "abc" }];
   checkResult( dbcl, findCondition19, hintConf, sortConf, expRecs19 );

   //$type + $all
   var findCondition20 = { a: { $type: 1, $all: [1] } };
   var expRecs20 = [{ a: 1001.02 }];
   checkResult( dbcl, findCondition20, hintConf, sortConf, expRecs20 );

   //$type + $isnull
   var findCondition1 = { a: { $type: 1, $isnull: 1 } };
   var expRecs1 = [{ b: 1 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$type + $exists
   var findCondition21 = { a: { $type: 1, $exists: 0 } };
   var expRecs21 = [{ b: 1 }];
   checkResult( dbcl, findCondition21, hintConf, sortConf, expRecs21 );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}