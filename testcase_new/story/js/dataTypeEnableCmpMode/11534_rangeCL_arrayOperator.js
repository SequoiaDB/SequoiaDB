/************************************
*@Description: match range sharding, use $expand/$returnMatch
data type: int/numberLong/double/decimal/string/bool/date/timestamp/binary/regex/json/array/null/minKey/maxKey
*@author:      liuxiaoxuan
*@createdate:  2017.5.23
*@testlinkCase: seqDB-11534
**************************************/

main( test );
function test ()
{
   //set find data from master
   db.setSessionAttr( { PreferedInstance: "M" } );

   var clName = COMMCLNAME + "_11534";
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
   var doc = [{ a: 10 }, { a: 101 }, { a: 999 },
   { a: { $decimal: "123.456" } }, { a: 20170523.23 },
   { a: { $numberLong: "10000" } }, { a: { $numberLong: "20002" } },
   { a: { $date: "2017-05-01" } }, { a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "abc" },
   { a: MinKey() },
   { a: MaxKey() },
   { a: true }, { a: false },
   { a: { name: "Jack" } },
   { a: [1] },
   { a: [3] },
   { a: [101] },
   { a: [999] },
   { a: ["abc"] },
   { b: 1 }];
   dbcl.insert( doc );

   //split cl
   var startCondition1 = { a: 1 };
   var endCondition1 = { a: 100 };
   ClSplitOneTimes( COMMCSNAME, clName, startCondition1, endCondition1 );

   //$expand
   var findCondition1 = { a: { $expand: 1 } };
   var expRecs1 = [{ a: 10 },
   { a: 101 },
   { a: 999 },
   { a: { $decimal: "123.456" } },
   { a: 20170523.23 },
   { a: 10000 }, { a: 20002 },
   { a: { $date: "2017-05-01" } },
   { a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "abc" },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: true }, { a: false },
   { a: { name: "Jack" } },
   { a: 1 },
   { a: 3 },
   { a: 101 },
   { a: 999 },
   { a: "abc" },
   { b: 1 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );


   var findCondition2 = { a: { $expand: 1, $gt: 0 } };
   var expRecs2 = [{ a: 10 }, { a: 101 }, { a: 999 },
   { a: { $decimal: "123.456" } }, { a: 20170523.23 },
   { a: 10000 }, { a: 20002 },
   { a: 1 },
   { a: 3 },
   { a: 101 },
   { a: 999 }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   //returnMatch
   var findCondition3 = { a: { $returnMatch: 0, $in: [1, 3, 101, "abc"] } };
   var expRecs3 = [{ a: 101 },
   { a: "abc" },
   { a: [1] },
   { a: [3] },
   { a: [101] },
   { a: ["abc"] }];
   checkResult( dbcl, findCondition3, hintConf, sortConf, expRecs3 );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}
