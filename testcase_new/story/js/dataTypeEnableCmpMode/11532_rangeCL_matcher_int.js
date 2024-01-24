/************************************
*@Description: match range sharding, use et/gt/gte/lt/lte/ne/et/mod/in/isnull/nin/all/and/not/or/type/exists/elemMatch/size/regex/field/expand/returnMatch
data type: int/numberLong/double/decimal/string/bool/date/timestamp/binary/regex/json/array/null/minKey/maxKey
*@author:      liuxiaoxuan
*@createdate:  2017.5.19
*@testlinkCase: seqDB-11532
**************************************/


main( test );
function test ()
{
   //set find data from master
   db.setSessionAttr( { PreferedInstance: "M" } );

   var clName = COMMCLNAME + "_11532";
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
   var doc = [{ a: -10 }, { a: -5 }, { a: 0 }, { a: 5 }, { a: 10 },
   { a: [-10] }, { a: [-5] }, { a: [0] }, { a: [5] }, { a: [10] },
   { a: [[-10]] }, { a: [[-5]] }, { a: [[0]] }, { a: [[5]] }, { a: [[10]] },
   { a: { $date: "2017-05-01" } },
   { a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "abc" },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: true },
   { a: { name: "Jack" } },
   { b: 1 },
   { a: -34, b: -34 },
   { a: -34, b: -33 },
   { a: -34, b: -35 },
   { a: 35, b: 34 },
   { a: 35, b: 35 },
   { a: 35, b: 36 }];
   dbcl.insert( doc );

   //split cl
   var startCondition1 = { a: 0 };
   ClSplitOneTimes( COMMCSNAME, clName, startCondition1, null );

   //$et
   var findCondition1 = { a: { $et: 0 } };
   var expRecs1 = [{ a: 0 }, { a: [0] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //SEQUOIADBMAINSTREAM-2468
   var findCondition1 = { a: { $et: [0] } };
   var expRecs1 = [{ a: [0] }, { a: [[0]] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition2 = { a: { $et: { $date: "2017-05-01" } } };
   var expRecs2 = [{ a: { $date: "2017-05-01" } }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   var findCondition3 = { a: { $et: { $oid: "123abcd00ef12358902300ef" } } };
   var expRecs3 = [{ a: { $oid: "123abcd00ef12358902300ef" } }];
   checkResult( dbcl, findCondition3, hintConf, sortConf, expRecs3 );

   var findCondition4 = { a: { $et: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } } };
   var expRecs4 = [{ a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } }];
   checkResult( dbcl, findCondition4, hintConf, sortConf, expRecs4 );

   var findCondition5 = { a: { $et: "abc" } };
   var expRecs5 = [{ a: "abc" }];
   checkResult( dbcl, findCondition5, hintConf, sortConf, expRecs5 );

   var findCondition6 = { a: { $et: { name: "Jack" } } };
   var expRecs6 = [{ a: { name: "Jack" } }];
   checkResult( dbcl, findCondition6, hintConf, sortConf, expRecs6 );

   var findCondition7 = { a: { $et: null } };
   var expRecs7 = [{ a: null }];
   checkResult( dbcl, findCondition7, hintConf, sortConf, expRecs7 );

   var findCondition8 = { a: { $et: true } };
   var expRecs8 = [{ a: true }];
   checkResult( dbcl, findCondition8, hintConf, sortConf, expRecs8 );

   //$gt
   var findCondition1 = { a: { $gt: 0 } };
   var expRecs1 = [{ a: 5 }, { a: 10 },
   { a: [5] }, { a: [10] },
   { a: 35, b: 34 },
   { a: 35, b: 35 },
   { a: 35, b: 36 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition1 = { a: { $gt: -10 } };
   var expRecs1 = [{ a: -5 }, { a: 0 }, { a: 5 }, { a: 10 },
   { a: [-5] }, { a: [0] }, { a: [5] }, { a: [10] },
   { a: 35, b: 34 },
   { a: 35, b: 35 },
   { a: 35, b: 36 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition1 = { a: { $gt: [-10] } };
   var expRecs1 = [{ a: [-5] }, { a: [0] }, { a: [5] }, { a: [10] }, { a: [[-10]] }, { a: [[-5]] }, { a: [[0]] }, { a: [[5]] }, { a: [[10]] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition1 = { $and: [{ a: { $gt: -10 } }, { a: { $lt: { $timestamp: "2017-05-01-15.32.18.000000" } } }] };
   var expRecs1 = [];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition1 = { a: { $gt: "a" } };
   var expRecs1 = [{ a: "abc" }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$gte
   var findCondition1 = { a: { $gte: 0 } };
   var expRecs1 = [{ a: 0 }, { a: 5 }, { a: 10 },
   { a: [0] }, { a: [5] }, { a: [10] },
   { a: 35, b: 34 },
   { a: 35, b: 35 },
   { a: 35, b: 36 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition1 = { a: { $gte: -5 } };
   var expRecs1 = [{ a: -5 }, { a: 0 }, { a: 5 }, { a: 10 },
   { a: [-5] }, { a: [0] }, { a: [5] }, { a: [10] },
   { a: 35, b: 34 },
   { a: 35, b: 35 },
   { a: 35, b: 36 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition1 = { a: { $gte: [-5] } };
   var expRecs1 = [{ a: [-5] }, { a: [0] }, { a: [5] }, { a: [10] }, { a: [[-10]] }, { a: [[-5]] }, { a: [[0]] }, { a: [[5]] }, { a: [[10]] },];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$lt
   var findCondition1 = { a: { $lt: 0 } };
   var expRecs1 = [{ a: -10 }, { a: -5 },
   { a: [-10] }, { a: [-5] },
   { a: -34, b: -34 },
   { a: -34, b: -33 },
   { a: -34, b: -35 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition1 = { a: { $lt: 10 } };
   var expRecs1 = [{ a: -10 }, { a: -5 }, { a: 0 }, { a: 5 },
   { a: [-10] }, { a: [-5] }, { a: [0] }, { a: [5] },
   { a: -34, b: -34 },
   { a: -34, b: -33 },
   { a: -34, b: -35 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition1 = { a: { $lt: [10] } };
   var expRecs1 = [{ a: [-10] }, { a: [-5] }, { a: [0] }, { a: [5] }, { a: [[-10]] }, { a: [[-5]] }, { a: [[0]] }, { a: [[5]] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$lte
   var findCondition1 = { a: { $lte: 0 } };
   var expRecs1 = [{ a: -10 }, { a: -5 }, { a: 0 },
   { a: [-10] }, { a: [-5] }, { a: [0] },
   { a: -34, b: -34 },
   { a: -34, b: -33 },
   { a: -34, b: -35 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition1 = { a: { $lte: 5 } };
   var expRecs1 = [{ a: -10 }, { a: -5 }, { a: 0 }, { a: 5 },
   { a: [-10] }, { a: [-5] }, { a: [0] }, { a: [5] },
   { a: -34, b: -34 },
   { a: -34, b: -33 },
   { a: -34, b: -35 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition1 = { a: { $lte: [5] } };
   var expRecs1 = [{ a: [-10] }, { a: [-5] }, { a: [0] }, { a: [5] }, { a: [[-10]] }, { a: [[-5]] }, { a: [[0]] }, { a: [[5]] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$ne
   var findCondition1 = { a: { $ne: 0 } };
   var expRecs1 = [{ a: -10 }, { a: -5 }, { a: 5 }, { a: 10 },
   { a: [-10] }, { a: [-5] }, { a: [5] }, { a: [10] },
   { a: [[-10]] }, { a: [[-5]] }, { a: [[0]] }, { a: [[5]] }, { a: [[10]] },
   { a: { $date: "2017-05-01" } },
   { a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "abc" },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: true },
   { a: { name: "Jack" } },
   { a: -34, b: -34 },
   { a: -34, b: -33 },
   { a: -34, b: -35 },
   { a: 35, b: 34 },
   { a: 35, b: 35 },
   { a: 35, b: 36 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition1 = { a: { $ne: [0] } };
   var expRecs1 = [{ a: -10 }, { a: -5 }, { a: 0 }, { a: 5 }, { a: 10 },
   { a: [-10] }, { a: [-5] }, { a: [5] }, { a: [10] },
   { a: [[-10]] }, { a: [[-5]] }, { a: [[5]] }, { a: [[10]] },
   { a: { $date: "2017-05-01" } },
   { a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "abc" },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: true },
   { a: { name: "Jack" } },
   { a: -34, b: -34 },
   { a: -34, b: -33 },
   { a: -34, b: -35 },
   { a: 35, b: 34 },
   { a: 35, b: 35 },
   { a: 35, b: 36 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$mod
   var findCondition1 = { a: { $mod: [2, 0] } };
   var expRecs1 = [{ a: -10 }, { a: 0 }, { a: 10 },
   { a: [-10] }, { a: [0] }, { a: [10] },
   { a: -34, b: -34 },
   { a: -34, b: -33 },
   { a: -34, b: -35 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$in
   var findCondition1 = { a: { $in: [-5, 0, 5] } };
   var expRecs1 = [{ a: -5 }, { a: 0 }, { a: 5 },
   { a: [-5] }, { a: [0] }, { a: [5] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$nin
   var findCondition1 = { a: { $nin: [-5, 0, 5] } };
   var expRecs1 = [{ a: -10 }, { a: 10 },
   { a: [-10] }, { a: [10] },
   { a: [[-10]] }, { a: [[-5]] }, { a: [[0]] }, { a: [[5]] }, { a: [[10]] },
   { a: { $date: "2017-05-01" } },
   { a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "abc" },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: true },
   { a: { name: "Jack" } },
   { a: -34, b: -34 },
   { a: -34, b: -33 },
   { a: -34, b: -35 },
   { a: 35, b: 34 },
   { a: 35, b: 35 },
   { a: 35, b: 36 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$isnull
   var findCondition1 = { a: { $isnull: 0 } };
   var expRecs1 = [{ a: -10 }, { a: -5 }, { a: 0 }, { a: 5 }, { a: 10 },
   { a: [-10] }, { a: [-5] }, { a: [0] }, { a: [5] }, { a: [10] },
   { a: [[-10]] }, { a: [[-5]] }, { a: [[0]] }, { a: [[5]] }, { a: [[10]] },
   { a: { $date: "2017-05-01" } },
   { a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "abc" },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: true },
   { a: { name: "Jack" } },
   { a: -34, b: -34 },
   { a: -34, b: -33 },
   { a: -34, b: -35 },
   { a: 35, b: 34 },
   { a: 35, b: 35 },
   { a: 35, b: 36 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition1 = { a: { $isnull: 1 } };
   var expRecs1 = [{ a: null }, { b: 1 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$all
   var findCondition1 = { a: { $all: [5] } };
   var expRecs1 = [{ a: 5 }, { a: [5] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$and
   var findCondition1 = { $and: [{ a: { $gt: -10 } }, { a: { $lt: 10 } }] };
   var expRecs1 = [{ a: -5 }, { a: 0 }, { a: 5 },
   { a: [-5] }, { a: [0] }, { a: [5] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$not
   var findCondition1 = { $not: [{ a: 10 }] };
   var expRecs1 = [{ a: -10 }, { a: -5 }, { a: 0 }, { a: 5 },
   { a: [-10] }, { a: [-5] }, { a: [0] }, { a: [5] },
   { a: [[-10]] }, { a: [[-5]] }, { a: [[0]] }, { a: [[5]] }, { a: [[10]] },
   { a: { $date: "2017-05-01" } },
   { a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "abc" },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: true },
   { a: { name: "Jack" } },
   { b: 1 },
   { a: -34, b: -34 },
   { a: -34, b: -33 },
   { a: -34, b: -35 },
   { a: 35, b: 34 },
   { a: 35, b: 35 },
   { a: 35, b: 36 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$or
   var findCondition2 = { $or: [{ a: 10 }, { a: { $lt: 0 } }] };
   var expRecs2 = [{ a: -10 }, { a: -5 }, { a: 10 },
   { a: [-10] }, { a: [-5] }, { a: [10] },
   { a: -34, b: -34 },
   { a: -34, b: -33 },
   { a: -34, b: -35 }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   //$exists
   var findCondition3 = { a: { $exists: 1 } };
   var expRecs3 = [{ a: -10 }, { a: -5 }, { a: 0 }, { a: 5 }, { a: 10 },
   { a: [-10] }, { a: [-5] }, { a: [0] }, { a: [5] }, { a: [10] },
   { a: [[-10]] }, { a: [[-5]] }, { a: [[0]] }, { a: [[5]] }, { a: [[10]] },
   { a: { $date: "2017-05-01" } },
   { a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "abc" },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: true },
   { a: { name: "Jack" } },
   { a: -34, b: -34 },
   { a: -34, b: -33 },
   { a: -34, b: -35 },
   { a: 35, b: 34 },
   { a: 35, b: 35 },
   { a: 35, b: 36 }];
   checkResult( dbcl, findCondition3, hintConf, sortConf, expRecs3 );

   var findCondition3 = { a: { $exists: 0 } };
   var expRecs3 = [{ b: 1 }];
   checkResult( dbcl, findCondition3, hintConf, sortConf, expRecs3 );

   //$elemMatch
   var findCondition4 = { a: { $elemMatch: { name: "Jack" } } };
   var expRecs4 = [{ a: { name: "Jack" } }];
   checkResult( dbcl, findCondition4, hintConf, sortConf, expRecs4 );

   //$field
   var findCondition5 = { a: { $field: "b" } };
   var expRecs5 = [{ a: -34, b: -34 },
   { a: 35, b: 35 }];
   checkResult( dbcl, findCondition5, hintConf, sortConf, expRecs5 );

   var findCondition5 = { a: { $et: { $field: "b" } } };
   var expRecs5 = [{ a: -34, b: -34 },
   { a: 35, b: 35 }];
   checkResult( dbcl, findCondition5, hintConf, sortConf, expRecs5 );

   var findCondition5 = { a: { $gt: { $field: "b" } } };
   var expRecs5 = [{ a: -34, b: -35 },
   { a: 35, b: 34 }];
   checkResult( dbcl, findCondition5, hintConf, sortConf, expRecs5 );

   var findCondition5 = { a: { $gte: { $field: "b" } } };
   var expRecs5 = [{ a: -34, b: -34 },
   { a: -34, b: -35 },
   { a: 35, b: 34 },
   { a: 35, b: 35 }];
   checkResult( dbcl, findCondition5, hintConf, sortConf, expRecs5 );

   var findCondition5 = { a: { $lt: { $field: "b" } } };
   var expRecs5 = [{ a: -34, b: -33 },
   { a: 35, b: 36 }];
   checkResult( dbcl, findCondition5, hintConf, sortConf, expRecs5 );

   var findCondition5 = { a: { $lte: { $field: "b" } } };
   var expRecs5 = [{ a: -34, b: -34 },
   { a: -34, b: -33 },
   { a: 35, b: 35 },
   { a: 35, b: 36 }];
   checkResult( dbcl, findCondition5, hintConf, sortConf, expRecs5 );

   var findCondition5 = { a: { $ne: { $field: "b" } } };
   var expRecs5 = [{ a: -34, b: -33 },
   { a: -34, b: -35 },
   { a: 35, b: 34 },
   { a: 35, b: 36 }];
   checkResult( dbcl, findCondition5, hintConf, sortConf, expRecs5 );

   //$expand
   var findCondition6 = { a: { $expand: 1 } };
   var expRecs6 = [{ a: -10 }, { a: -5 }, { a: 0 }, { a: 5 }, { a: 10 },
   { a: -10 }, { a: -5 }, { a: 0 }, { a: 5 }, { a: 10 },
   { a: [-10] }, { a: [-5] }, { a: [0] }, { a: [5] }, { a: [10] },
   { a: { $date: "2017-05-01" } },
   { a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "abc" },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: true },
   { a: { name: "Jack" } },
   { b: 1 },
   { a: -34, b: -34 },
   { a: -34, b: -33 },
   { a: -34, b: -35 },
   { a: 35, b: 34 },
   { a: 35, b: 35 },
   { a: 35, b: 36 }];
   checkResult( dbcl, findCondition6, hintConf, sortConf, expRecs6 );

   //$returnMatch
   var findCondition7 = { a: { $returnMatch: 0, $in: [-10, 0, 10] } };
   var expRecs7 = [{ a: -10 }, { a: 0 }, { a: 10 },
   { a: [-10] }, { a: [0] }, { a: [10] }];
   checkResult( dbcl, findCondition7, hintConf, sortConf, expRecs7 );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}