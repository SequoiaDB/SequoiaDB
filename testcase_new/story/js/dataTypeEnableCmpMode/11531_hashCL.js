/************************************
*@Description: match hash sharding, use et/gt, 
data type: int/numberLong/double/decimal/string/bool/date/timestamp/binary/regex/json/array/null/minKey/maxKey
*@author:      liuxiaoxuan
*@createdate:  2017.5.18
*@testlinkCase: seqDB-11531
**************************************/

main( test );
function test ()
{
   //set find data from master
   db.setSessionAttr( { PreferedInstance: "M" } );
   var clName = COMMCLNAME + "_11531";

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

   //create cl for hash split
   var ClOption = { ShardingKey: { "a": 1 }, ShardingType: "hash" };
   var dbcl = commCreateCL( db, COMMCSNAME, clName, ClOption, true, true );

   var hintConf = [{ "": null }, { "": "$shard" }];
   var sortConf = { _id: 1 };

   //insert data
   var doc = [{ a: 100 }, { a: 200 }, { a: 10000 },
   { a: { $numberLong: "10000001" } },
   { a: { $numberLong: "20000002" } },
   { a: { $numberLong: "30000003" } },
   { a: 1001.01 }, { a: 2002.02 }, { a: 3003.03 },
   { a: { $date: "2017-03-20" } },
   { a: { $date: "2017-04-28" } },
   { a: { $date: "2017-05-10" } },
   { a: { $timestamp: "2017-03-20-13.14.15.000000" } },
   { a: { $timestamp: "2017-04-28-13.14.15.000000" } },
   { a: { $timestamp: "2017-05-10-13.14.15.000000" } },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", "$type": "1" } },
   { a: { $regex: "^z", "$options": "i" } },
   { a: null },
   { a: "aa" }, { a: "Ab" }, { a: "aB" }, { a: "abc" }, { a: "bA" },
   { a: MinKey() },
   { a: MaxKey() },
   { a: { $decimal: "20170516.01" } },
   { a: { $decimal: "20170517.02" } },
   { a: { $decimal: "20170518.03" } },
   { a: true }, { a: false },
   { a: { name: "Jack" } },
   { a: [1] },
   { a: [3] },
   { a: 34, b: 34 },
   { a: 34, b: 33 },
   { a: 34, b: 35 },
   { a: 35, b: 34 },
   { a: 35, b: 35 },
   { a: 35, b: 36 },
   { b: 37 },
   { c: 37 }];
   dbcl.insert( doc );

   //split cl
   var startCondition = 50;
   var splitGrInfo = ClSplitOneTimes( COMMCSNAME, clName, startCondition, null );

   //$et
   var findCondition1 = { a: { $et: 100 } };
   var expRecs1 = [{ a: 100 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$gt
   var findCondition2 = { a: { $gt: 1 } };
   var expRecs2 = [{ a: 100 }, { a: 200 }, { a: 10000 },
   { a: 10000001 }, { a: 20000002 }, { a: 30000003 },
   { a: 1001.01 }, { a: 2002.02 }, { a: 3003.03 },
   { a: { $decimal: "20170516.01" } },
   { a: { $decimal: "20170517.02" } },
   { a: { $decimal: "20170518.03" } },
   { a: [3] },
   { a: 34, b: 34 },
   { a: 34, b: 33 },
   { a: 34, b: 35 },
   { a: 35, b: 34 },
   { a: 35, b: 35 },
   { a: 35, b: 36 }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   //regex, SEQUOIADBMAINSTREAM-2460
   var findCondition3 = { a: { $regex: "^a", $options: "i" } };
   var expRecs3 = [{ a: "aa" }, { a: "Ab" }, { a: "aB" }, { a: "abc" }];
   checkResult( dbcl, findCondition3, hintConf, sortConf, expRecs3 );

   var findCondition3 = { a: { $options: "i", $regex: "^a" } };
   var expRecs3 = [{ a: "aa" }, { a: "Ab" }, { a: "aB" }, { a: "abc" }];
   checkResult( dbcl, findCondition3, hintConf, sortConf, expRecs3 );

   var findCondition3 = { a: { $regex: "^a" } };
   var expRecs3 = [{ a: "aa" }, { a: "aB" }, { a: "abc" }];
   checkResult( dbcl, findCondition3, hintConf, sortConf, expRecs3 );

   //field, et
   var findCondition4 = { a: { $field: "b" } };
   var expRecs4 = [{ a: 34, b: 34 },
   { a: 35, b: 35 }];
   checkResult( dbcl, findCondition4, hintConf, sortConf, expRecs4 );

   var findCondition5 = { a: { $et: { $field: "b" } } };
   var expRecs5 = [{ a: 34, b: 34 },
   { a: 35, b: 35 }];
   checkResult( dbcl, findCondition5, hintConf, sortConf, expRecs5 );

   //field, gt
   var findCondition6 = { a: { $gt: { $field: "b" } } };
   var expRecs6 = [{ a: 34, b: 33 },
   { a: 35, b: 34 }];
   checkResult( dbcl, findCondition6, hintConf, sortConf, expRecs6 );

   //field, gte
   var findCondition7 = { a: { $gte: { $field: "b" } } };
   var expRecs7 = [{ a: 34, b: 34 },
   { a: 34, b: 33 },
   { a: 35, b: 34 },
   { a: 35, b: 35 }];
   checkResult( dbcl, findCondition7, hintConf, sortConf, expRecs7 );

   //field, lt
   var findCondition8 = { a: { $lt: { $field: "b" } } };
   var expRecs8 = [{ a: 34, b: 35 },
   { a: 35, b: 36 }];
   checkResult( dbcl, findCondition8, hintConf, sortConf, expRecs8 );

   //field, lte
   var findCondition9 = { a: { $lte: { $field: "b" } } };
   var expRecs9 = [{ a: 34, b: 34 },
   { a: 34, b: 35 },
   { a: 35, b: 35 },
   { a: 35, b: 36 }];
   checkResult( dbcl, findCondition9, hintConf, sortConf, expRecs9 );

   //field, ne
   var findCondition10 = { a: { $ne: { $field: "b" } } };
   var expRecs10 = [{ a: 34, b: 33 },
   { a: 34, b: 35 },
   { a: 35, b: 34 },
   { a: 35, b: 36 }];
   checkResult( dbcl, findCondition10, hintConf, sortConf, expRecs10 );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}