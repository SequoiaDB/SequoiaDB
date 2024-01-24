/************************************
*@Description: match range sharding
data type: int/numberLong/double/decimal/string/bool/date/timestamp/binary/regex/json/array/null/minKey/maxKey
*@author:      liuxiaoxuan
*@createdate:  2017.5.24
*@testlinkCase:seqDB-11537
**************************************/
main( test );
function test ()
{
   //set find data from master
   db.setSessionAttr( { PreferedInstance: "M" } );
   var hintConf = [{ "": "$shard" }, { "": null }];
   var sortConf = { _id: 1 };

   var clName = COMMCLNAME + "_11537";
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


   var ClOption = { ShardingKey: { "a": 1, "b": 1, "c": 1 }, ShardingType: "range", ReplSize: 0 };
   var dbcl = commCreateCL( db, COMMCSNAME, clName, ClOption, true, true );

   var hintConf = [{ "": null }, { "": "$shard" }];
   var sortConf = { _id: 1 };

   var startCondition1 = { a: 0, b: 0, c: 0 };
   ClSplitOneTimes( COMMCSNAME, clName, startCondition1, null );

   //insert data
   var doc = [{ a: { $minKey: 1 }, b: { $minKey: 1 }, c: { $minKey: 1 } },
   { d: 1 },
   { a: null, b: null, c: null },
   { a: -22, b: -22, c: -22 },
   { a: -2, b: -2, c: -2 },
   { a: 0, b: 0, c: 0 },
   { a: 24, b: 24, c: 24 },
   { a: "aa", b: "aa", c: "aa" },
   { a: "bb", b: "bb", c: "bb" },
   { a: "cc", b: "cc", c: "cc" },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, b: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" }, c: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $oid: "591cf397a54fe50425000000" }, b: { $oid: "591cf397a54fe50425000000" }, c: { $oid: "591cf397a54fe50425000000" } },
   { a: true, b: true, c: true },
   { a: { $date: "2014-01-01" }, b: { $date: "2014-01-01" }, c: { $date: "2014-01-01" } },
   { a: { $timestamp: "2013-06-05-16.10.33.000000" }, b: { $timestamp: "2013-06-05-16.10.33.000000" }, c: { $timestamp: "2013-06-05-16.10.33.000000" } },
   { a: { $regex: "^a", $options: "i" }, b: { $regex: "^a", $options: "i" }, c: { $regex: "^a", $options: "i" } },
   { a: { $maxKey: 1 }, b: { $maxKey: 1 }, c: { $maxKey: 1 } }];
   dbcl.insert( doc );

   //exists
   var findCondition1 = { $and: [{ a: { $exists: 0 } }, { b: { $exists: 0 } }, { c: { $exists: 0 } }] };
   var expRecs1 = [{ d: 1 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //isnull
   var findCondition2 = { $and: [{ a: { $isnull: 1 } }, { b: { $isnull: 1 } }, { c: { $isnull: 1 } }] };
   var expRecs2 = [{ d: 1 },
   { a: null, b: null, c: null }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   //gte
   var findCondition2 = { $and: [{ a: { $gte: -10 } }, { b: { $gte: -10 } }, { c: { $gte: -10 } }] };
   var expRecs2 = [{ a: -2, b: -2, c: -2 },
   { a: 0, b: 0, c: 0 },
   { a: 24, b: 24, c: 24 }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   var findCondition2 = { $and: [{ a: { $gte: "a" } }, { b: { $gte: "a" } }, { c: { $gte: "a" } }] };
   var expRecs2 = [{ a: "aa", b: "aa", c: "aa" },
   { a: "bb", b: "bb", c: "bb" },
   { a: "cc", b: "cc", c: "cc" }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   //gt
   var findCondition2 = { $and: [{ a: { $gte: -2 } }, { b: { $gte: -2 } }, { c: { $gte: -2 } }] };
   var expRecs2 = [{ a: -2, b: -2, c: -2 },
   { a: 0, b: 0, c: 0 },
   { a: 24, b: 24, c: 24 }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   //lte
   var findCondition2 = { $and: [{ a: { $lte: 0 } }, { b: { $lte: 0 } }, { c: { $lte: 0 } }] };
   var expRecs2 = [{ a: -22, b: -22, c: -22 },
   { a: -2, b: -2, c: -2 },
   { a: 0, b: 0, c: 0 }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   var findCondition2 = { $and: [{ a: { $lte: "c" } }, { b: { $lte: "c" } }, { c: { $lte: "c" } }] };
   var expRecs2 = [{ a: "aa", b: "aa", c: "aa" },
   { a: "bb", b: "bb", c: "bb" }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   //lt
   var findCondition2 = { $and: [{ a: { $lt: 1 } }, { b: { $lt: 1 } }, { c: { $lt: 1 } }] };
   var expRecs2 = [{ a: -22, b: -22, c: -22 },
   { a: -2, b: -2, c: -2 },
   { a: 0, b: 0, c: 0 }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}