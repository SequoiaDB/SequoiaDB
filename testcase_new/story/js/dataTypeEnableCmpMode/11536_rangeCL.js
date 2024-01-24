/************************************
*@Description: match range sharding, use et/gt/expand/returnMatch
data type: int/numberLong/double/decimal/string/bool/date/timestamp/binary/regex/json/array/null/minKey/maxKey
*@author:      liuxiaoxuan
*@createdate:  2017.5.24
*@testlinkCase: seqDB-11536
**************************************/

main( test );
function test ()
{
   //set find data from master
   db.setSessionAttr( { PreferedInstance: "M" } );

   var clName = COMMCLNAME + "_11536";
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
   var doc = [{ a: -99, b: 0 },
   { a: 0, b: 100 },
   { a: 99, b: 2017.05 },
   { a: 101, b: 1001.01 },
   { a: -100, b: { $decimal: "123.456" } },
   { a: 100, b: { $decimal: "567.789" } },
   { a: -101.01, b: { $numberLong: "10004" } },
   { a: 101.01, b: { $numberLong: "20002" } },
   { a: 11, b: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: -101, b: { $regex: "^z", $options: "i" } },
   { a: 101, b: null },
   { a: -1000, b: { $date: "2017-05-02" } },
   { a: 1, b: { $date: "2017-05-20" } },
   { a: 1000, b: { $date: "2017-05-21" } },
   { a: -1001, b: { $timestamp: "2017-05-02-15.32.18.000000" } },
   { a: -1, b: { $timestamp: "2017-05-02-20.32.18.000000" } },
   { a: 1001, b: { $timestamp: "2017-05-21-20.32.18.000000" } },
   { a: 1001.01, b: { $oid: "123abcd00ef12358902300ef" } },
   { a: -10, b: "abc" },
   { a: 5, b: "bcd" },
   { a: 10, b: "cde" },
   { a: 50, b: { name: "Jack" } },
   { a: [10], b: true },
   { a: [20], b: false },
   { a: [0], b: [0] },
   { a: [-98], b: [1, 2, 3] },
   { a: [98], b: [2, 3, 4, 5] },
   { a: [-99], b: ["a", "b", "c"] },
   { a: [99], b: ["c", "d", "e"] }];
   dbcl.insert( doc );

   //split cl
   var startCondition1 = { a: 0 };
   ClSplitOneTimes( COMMCSNAME, clName, startCondition1, null );

   //$et
   var findCondition1 = { b: { $et: 0 } };
   var expRecs1 = [{ a: -99, b: 0 }, { a: [0], b: [0] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition2 = { b: { $et: [0] } };
   var expRecs2 = [{ a: [0], b: [0] }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   var findCondition3 = { b: { $et: { $date: "2017-05-02" } } };
   var expRecs3 = [{ a: -1000, b: { $date: "2017-05-02" } }];
   checkResult( dbcl, findCondition3, hintConf, sortConf, expRecs3 );

   //$gt
   var findCondition4 = { b: { $gt: 0 } };
   var expRecs4 = [{ a: 0, b: 100 },
   { a: 99, b: 2017.05 }, { a: 101, b: 1001.01 },
   { a: -100, b: { $decimal: "123.456" } }, { a: 100, b: { $decimal: "567.789" } },
   { a: -101.01, b: 10004 }, { a: 101.01, b: 20002 },
   { a: [-98], b: [1, 2, 3] }, { a: [98], b: [2, 3, 4, 5] }];
   checkResult( dbcl, findCondition4, hintConf, sortConf, expRecs4 );

   var findCondition5 = { b: { $gt: "ab" } };
   var expRecs5 = [{ a: -10, b: "abc" }, { a: 5, b: "bcd" }, { a: 10, b: "cde" },
   { a: [-99], b: ["a", "b", "c"] }, { a: [99], b: ["c", "d", "e"] }];
   checkResult( dbcl, findCondition5, hintConf, sortConf, expRecs5 );

   var findCondition6 = { b: { $gt: [0] } };
   var expRecs6 = [{ a: [-98], b: [1, 2, 3] }, { a: [98], b: [2, 3, 4, 5] },
   { a: [-99], b: ["a", "b", "c"] }, { a: [99], b: ["c", "d", "e"] }];
   checkResult( dbcl, findCondition6, hintConf, sortConf, expRecs6 );

   var findCondition7 = { b: { $gt: { $date: "2017-05-02" } } };
   var expRecs7 = [{ a: 1, b: { $date: "2017-05-20" } },
   { a: 1000, b: { $date: "2017-05-21" } },
   { a: -1001, b: { $timestamp: "2017-05-02-15.32.18.000000" } },
   { a: -1, b: { $timestamp: "2017-05-02-20.32.18.000000" } },
   { a: 1001, b: { $timestamp: "2017-05-21-20.32.18.000000" } }];
   checkResult( dbcl, findCondition7, hintConf, sortConf, expRecs7 );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}