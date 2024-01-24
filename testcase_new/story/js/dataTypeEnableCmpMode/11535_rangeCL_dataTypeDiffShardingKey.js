/************************************
*@Description: match range sharding, use gt/gte/lt/lte/et/ne/mod/in/all/{isnull:1}/{exists:0}/regex
data type: int/numberLong/double/decimal/string/bool/date/timestamp/binary/regex/json/array/null/minKey/maxKey
*@author:      liuxiaoxuan
*@createdate:  2017.5.23
*@testlinkCase: seqDB-11535
**************************************/
main( test );
function test ()
{
   //set find data from master
   db.setSessionAttr( { PreferedInstance: "M" } );

   var clName = COMMCLNAME + "_11535";
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


   var ClOption = { ShardingKey: { "a": 1 }, ShardingType: "range", ReplSize: 0 };
   var dbcl = commCreateCL( db, COMMCSNAME, clName, ClOption, true, true );

   var hintConf = [{ "": null }, { "": "$shard" }];
   var sortConf = { _id: 1 };

   //insert data
   var doc = [{ a: -100 }, { a: -10 }, { a: 0 }, { a: 10 }, { a: 100 },
   { a: { $decimal: "123.456" } },
   { a: 2017.05 },
   { a: { $numberLong: "10000" } },
   { a: { $numberLong: "20002" } },
   { a: { $date: "2017-05-02" } },
   { a: { $timestamp: "2017-05-02-10.11.12.000000" } },
   { a: { $date: "2017-05-10" } },
   { a: { $timestamp: "2017-05-10-13.14.15.000000" } },
   { a: { $date: "2017-06-01" } },
   { a: { $timestamp: "2017-06-01-16.17.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "aa" }, { a: "ab" }, { a: "abc" },
   { a: MinKey() },
   { a: MaxKey() },
   { a: true }, { a: false },
   { a: { name: "Jack" } },
   { a: [10] },
   { a: [20] },
   { a: [100] },
   { b: 1 },
   { b: "a" },
   { b: null }];
   dbcl.insert( doc );

   //split cl
   var startCondition1 = { a: 0 };
   ClSplitOneTimes( COMMCSNAME, clName, startCondition1, null );

   //$et
   var findCondition1 = { a: { $et: "abc" } };
   var expRecs1 = [{ a: "abc" }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$ne
   var findCondition1 = { a: { $ne: "abc" } };
   var expRecs1 = [{ a: -100 }, { a: -10 }, { a: 0 }, { a: 10 }, { a: 100 },
   { a: { $decimal: "123.456" } }, { a: 2017.05 },
   { a: 10000 }, { a: 20002 },
   { a: { $date: "2017-05-02" } }, { a: { $timestamp: "2017-05-02-10.11.12.000000" } },
   { a: { $date: "2017-05-10" } }, { a: { $timestamp: "2017-05-10-13.14.15.000000" } },
   { a: { $date: "2017-06-01" } }, { a: { $timestamp: "2017-06-01-16.17.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "aa" }, { a: "ab" },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: true }, { a: false },
   { a: { name: "Jack" } },
   { a: [10] },
   { a: [20] },
   { a: [100] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$gt
   var findCondition1 = { a: { $gt: { $date: "2017-05-02" } } };
   var expRecs1 = [{ a: { $timestamp: "2017-05-02-10.11.12.000000" } },
   { a: { $date: "2017-05-10" } },
   { a: { $timestamp: "2017-05-10-13.14.15.000000" } },
   { a: { $date: "2017-06-01" } },
   { a: { $timestamp: "2017-06-01-16.17.18.000000" } }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$gte
   var findCondition1 = { a: { $gte: { $date: "2017-05-02" } } };
   var expRecs1 = [{ a: { $date: "2017-05-02" } },
   { a: { $timestamp: "2017-05-02-10.11.12.000000" } },
   { a: { $date: "2017-05-10" } },
   { a: { $timestamp: "2017-05-10-13.14.15.000000" } },
   { a: { $date: "2017-06-01" } },
   { a: { $timestamp: "2017-06-01-16.17.18.000000" } }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition2 = { a: { $gte: "a" } };
   var expRecs2 = [{ a: "aa" },
   { a: "ab" },
   { a: "abc" }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   //$lt
   var findCondition1 = { a: { $lt: { $date: "2017-06-01" } } };
   var expRecs1 = [{ a: { $date: "2017-05-02" } },
   { a: { $timestamp: "2017-05-02-10.11.12.000000" } },
   { a: { $date: "2017-05-10" } },
   { a: { $timestamp: "2017-05-10-13.14.15.000000" } }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$lte
   var findCondition1 = { a: { $lte: { $date: "2017-06-01" } } };
   var expRecs1 = [{ a: { $date: "2017-05-02" } },
   { a: { $timestamp: "2017-05-02-10.11.12.000000" } },
   { a: { $date: "2017-05-10" } },
   { a: { $timestamp: "2017-05-10-13.14.15.000000" } },
   { a: { $date: "2017-06-01" } }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition2 = { a: { $lte: "abc" } };
   var expRecs2 = [{ a: "aa" }, { a: "ab" }, { a: "abc" }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   //$mod
   var findCondition1 = { a: { $mod: [2, 0] } };
   var expRecs1 = [{ a: -100 },
   { a: -10 },
   { a: 0 },
   { a: 10 },
   { a: 100 },
   { a: 10000 },
   { a: 20002 },
   { a: [10] },
   { a: [20] },
   { a: [100] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$in
   var findCondition1 = {
      a: {
         $in: [{ $timestamp: "2017-05-02-10.11.12.000000" },
         { $date: "2017-05-10" }]
      }
   };
   var expRecs1 = [{ a: { $timestamp: "2017-05-02-10.11.12.000000" } },
   { a: { $date: "2017-05-10" } }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$all
   var findCondition1 = { a: { $all: [{ $timestamp: "2017-05-02-10.11.12.000000" }] } };
   var expRecs1 = [{ a: { $timestamp: "2017-05-02-10.11.12.000000" } }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$isnull
   var findCondition1 = { a: { $isnull: 1 } };
   var expRecs1 = [{ a: null },
   { b: 1 },
   { b: "a" },
   { b: null }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$exists
   var findCondition1 = { a: { $exists: 0 } };
   var expRecs1 = [{ b: 1 },
   { b: "a" },
   { b: null }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$regex
   var findCondition1 = { a: { $regex: "^a", $options: "i" } };
   var expRecs1 = [{ a: "aa" },
   { a: "ab" },
   { a: "abc" }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}