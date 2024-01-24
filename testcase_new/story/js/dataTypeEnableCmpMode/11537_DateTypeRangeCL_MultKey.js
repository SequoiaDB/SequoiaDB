/************************************
*@Description: match range sharding, use gt/gte/lt/lte/et/ne/mod/in/all/{isnull:1}/{exists:0}
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

   //clean environment before test
   mainCL_Name = "maincl_11537";
   subCL_Name1 = "subcl1_11537";
   subCL_Name2 = "subcl2_11537";
   subCL_Name3 = "subcl3_11537";

   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, subCL_Name3, true, true, "clean sub collection" );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection" );

   //check test environment before split

   //standalone can not split
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //less two groups, can not split
   var allGroupName = getGroupName( db );
   for( var m = 0; m < allGroupName.length; m++ )
   {
   }
   if( 1 === allGroupName.length )
   {
      return;
   }


   //create maincl for range split
   var mainCLOption = { ShardingKey: { "a": 1 }, ShardingType: "range", IsMainCL: true };
   var dbcl = commCreateCL( db, COMMCSNAME, mainCL_Name, mainCLOption, true, true );

   //create subcl
   var subClOption1 = { ShardingKey: { "b": 1 }, ShardingType: "range" };
   commCreateCL( db, COMMCSNAME, subCL_Name1, subClOption1, true, true );

   var subClOption2 = { ShardingKey: { "b": -1 }, ShardingType: "range" };
   commCreateCL( db, COMMCSNAME, subCL_Name2, subClOption2, true, true );

   commCreateCL( db, COMMCSNAME, subCL_Name3 );

   //split cl
   startCondition1 = { b: 0 };
   splitGrInfo = ClSplitOneTimes( COMMCSNAME, subCL_Name1, startCondition1, null );

   startCondition2 = { b: 0 };
   splitGrInfo = ClSplitOneTimes( COMMCSNAME, subCL_Name2, startCondition2, null );

   //attach subcl

   dbcl.attachCL( COMMCSNAME + "." + subCL_Name1, { LowBound: { a: -1000, b: -1000 }, UpBound: { a: 0, b: 0 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name2, { LowBound: { a: 0, b: 0 }, UpBound: { a: 1000, b: 1000 } } );
   dbcl.attachCL( COMMCSNAME + "." + subCL_Name3, { LowBound: { a: 1000, b: 1000 }, UpBound: { a: 2000, b: 2000 } } );


   //insert data
   var doc = [//subcl1
      { a: -1000, b: -1000, c: 100 },
      { a: -101, b: -101, c: 101.01 },
      { a: -201, b: -201, c: { $decimal: "123.456" } },
      { a: -301, b: -301, c: { $numberLong: "10000" } },
      { a: -401, b: -401, c: { $date: "2017-05-02" } },
      { a: -501, b: -501, c: { $timestamp: "2017-05-02-15.32.18.000000" } },
      { a: -601, b: -601, c: { $oid: "123abcd00ef12358902300ef" } },
      { a: -701, b: -701, c: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
      { a: -801, b: -801, c: "abc" },
      { a: -901, b: -901, c: [1, 2, 3] },
      { a: -999, b: -999, c: { $minKey: 1 } },
      //subcl2
      { a: 0, b: 0, c: 200 },
      { a: 201, b: 201, c: 202.02 },
      { a: 301, b: 301, c: { $decimal: "456.789" } },
      { a: 401, b: 401, c: { $numberLong: "20000" } },
      { a: 501, b: 501, c: { $date: "2017-05-03" } },
      { a: 601, b: 601, c: { $timestamp: "2017-05-03-15.32.18.000000" } },
      { a: 701, b: 701, c: "efg" },
      { a: 801, b: 801, c: [3, 4, 5, 6] },
      { a: 901, b: 901, c: null },
      { a: 998, b: 998, c: true }, { a: 999, b: 999, c: false },
      //subcl3
      { a: 1000, b: 1000, c: 1000 },
      { a: 1001, b: 1001, c: 1001.01 },
      { a: 1200, b: 1200, c: { $decimal: "2017.05" } },
      { a: 1300, b: 1300, c: { $numberLong: "30000" } },
      { a: 1400, b: 1400, c: { $date: "2017-05-04" } },
      { a: 1500, b: 1500, c: { $timestamp: "2017-05-04-15.32.18.000000" } },
      { a: 1600, b: 1600, c: "hkj" },
      { a: 1700, b: 1700, c: [6, 10, 20] },
      { a: 1800, b: 1800, c: { name: "Jack" } },
      { a: 1999, b: 1999, c: { $maxKey: 1 } }];

   dbcl.insert( doc );

   //gt
   var findCondition1 = { $and: [{ a: { $gt: 0 } }, { b: { $gt: 0 } }, { c: { $gt: 100 } }] };
   var expRecs1 = [//subcl2
      { a: 201, b: 201, c: 202.02 },
      { a: 301, b: 301, c: { $decimal: "456.789" } },
      { a: 401, b: 401, c: 20000 },
      //subcl3
      { a: 1000, b: 1000, c: 1000 },
      { a: 1001, b: 1001, c: 1001.01 },
      { a: 1200, b: 1200, c: { $decimal: "2017.05" } },
      { a: 1300, b: 1300, c: 30000 }]
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition2 = { $and: [{ a: { $gt: -999 } }, { b: { $gt: -999 } }, { c: { $gt: "ab" } }] };
   var expRecs2 = [//subcl1
      { a: -801, b: -801, c: "abc" },
      //subcl2
      { a: 701, b: 701, c: "efg" },
      //subcl3
      { a: 1600, b: 1600, c: "hkj" }]
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   var findCondition3 = { $and: [{ a: { $gt: -501 } }, { b: { $gt: -501 } }, { c: { $gt: { $date: "2017-05-01" } } }] };
   var expRecs3 = [//subcl1
      { a: -401, b: -401, c: { $date: "2017-05-02" } },
      //subcl2
      { a: 501, b: 501, c: { $date: "2017-05-03" } },
      { a: 601, b: 601, c: { $timestamp: "2017-05-03-15.32.18.000000" } },
      //subcl3
      { a: 1400, b: 1400, c: { $date: "2017-05-04" } },
      { a: 1500, b: 1500, c: { $timestamp: "2017-05-04-15.32.18.000000" } }]
   checkResult( dbcl, findCondition3, hintConf, sortConf, expRecs3 );

   //gte
   var findCondition1 = { $and: [{ a: { $gte: 0 } }, { b: { $gte: 0 } }, { c: { $gte: 100 } }] };
   var expRecs1 = [//subcl2
      { a: 0, b: 0, c: 200 },
      { a: 201, b: 201, c: 202.02 },
      { a: 301, b: 301, c: { $decimal: "456.789" } },
      { a: 401, b: 401, c: 20000 },
      //subcl3
      { a: 1000, b: 1000, c: 1000 },
      { a: 1001, b: 1001, c: 1001.01 },
      { a: 1200, b: 1200, c: { $decimal: "2017.05" } },
      { a: 1300, b: 1300, c: 30000 }]
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition2 = { $and: [{ a: { $gte: -801 } }, { b: { $gte: -801 } }, { c: { $gte: "abc" } }] };
   var expRecs2 = [//subcl1
      { a: -801, b: -801, c: "abc" },
      //subcl2
      { a: 701, b: 701, c: "efg" },
      //subcl3
      { a: 1600, b: 1600, c: "hkj" }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   var findCondition3 = { $and: [{ a: { $gte: -501 } }, { b: { $gte: -501 } }, { c: { $gte: { $date: "2017-05-02" } } }] };
   var expRecs3 = [//subcl1
      { a: -401, b: -401, c: { $date: "2017-05-02" } },
      { a: -501, b: -501, c: { $timestamp: "2017-05-02-15.32.18.000000" } },
      //subcl2
      { a: 501, b: 501, c: { $date: "2017-05-03" } },
      { a: 601, b: 601, c: { $timestamp: "2017-05-03-15.32.18.000000" } },
      //subcl3
      { a: 1400, b: 1400, c: { $date: "2017-05-04" } },
      { a: 1500, b: 1500, c: { $timestamp: "2017-05-04-15.32.18.000000" } }];
   checkResult( dbcl, findCondition3, hintConf, sortConf, expRecs3 );

   //lt
   var findCondition1 = { $and: [{ a: { $lt: 1000 } }, { b: { $lt: 1000 } }, { c: { $lt: 10000 } }] };
   var expRecs1 = [//subcl1
      { a: -1000, b: -1000, c: 100 },
      { a: -101, b: -101, c: 101.01 },
      { a: -201, b: -201, c: { $decimal: "123.456" } },
      { a: -901, b: -901, c: [1, 2, 3] },
      //subcl2
      { a: 0, b: 0, c: 200 },
      { a: 201, b: 201, c: 202.02 },
      { a: 301, b: 301, c: { $decimal: "456.789" } },
      { a: 801, b: 801, c: [3, 4, 5, 6] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition2 = { $and: [{ a: { $lt: 1500 } }, { b: { $lt: 1500 } }, { c: { $lt: { $date: "2017-05-10" } } }] };
   var expRecs2 = [//subcl1
      { a: -401, b: -401, c: { $date: "2017-05-02" } },
      { a: -501, b: -501, c: { $timestamp: "2017-05-02-15.32.18.000000" } },
      //subcl2
      { a: 501, b: 501, c: { $date: "2017-05-03" } },
      { a: 601, b: 601, c: { $timestamp: "2017-05-03-15.32.18.000000" } },
      //subcl3
      { a: 1400, b: 1400, c: { $date: "2017-05-04" } }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   //lte
   var findCondition1 = { $and: [{ a: { $lte: 1000 } }, { b: { $lte: 1000 } }, { c: { $lte: 10000 } }] };
   var expRecs1 = [//subcl1
      { a: -1000, b: -1000, c: 100 },
      { a: -101, b: -101, c: 101.01 },
      { a: -201, b: -201, c: { $decimal: "123.456" } },
      { a: -901, b: -901, c: [1, 2, 3] },
      //subcl2
      { a: 0, b: 0, c: 200 },
      { a: 201, b: 201, c: 202.02 },
      { a: 301, b: 301, c: { $decimal: "456.789" } },
      { a: 801, b: 801, c: [3, 4, 5, 6] },
      //subcl3
      { a: 1000, b: 1000, c: 1000 }];

   var findCondition2 = { $and: [{ a: { $lte: 1500 } }, { b: { $lte: 1500 } }, { c: { $lte: { $date: "2017-05-10" } } }] };
   var expRecs2 = [//subcl1
      { a: -401, b: -401, c: { $date: "2017-05-02" } },
      { a: -501, b: -501, c: { $timestamp: "2017-05-02-15.32.18.000000" } },
      //subcl2
      { a: 501, b: 501, c: { $date: "2017-05-03" } },
      { a: 601, b: 601, c: { $timestamp: "2017-05-03-15.32.18.000000" } },
      //subcl3
      { a: 1400, b: 1400, c: { $date: "2017-05-04" } },
      { a: 1500, b: 1500, c: { $timestamp: "2017-05-04-15.32.18.000000" } }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   //et
   var findCondition1 = { $and: [{ a: { $et: -1000 } }, { b: { $et: -1000 } }, { c: { $et: 100 } }] };
   var expRecs1 = [//subcl1
      { a: -1000, b: -1000, c: 100 }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   var findCondition2 = { $and: [{ a: { $et: 701 } }, { b: { $et: 701 } }, { c: { $et: "efg" } }] };
   var expRecs2 = [//subcl2
      { a: 701, b: 701, c: "efg" }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   var findCondition3 = { $and: [{ a: { $et: 1400 } }, { b: { $et: 1400 } }, { c: { $et: { $date: "2017-05-04" } } }] };
   var expRecs3 = [//subcl3
      { a: 1400, b: 1400, c: { $date: "2017-05-04" } }];
   checkResult( dbcl, findCondition3, hintConf, sortConf, expRecs3 );

   //ne
   var findCondition1 = { $and: [{ a: { $ne: -1000 } }, { b: { $ne: -1000 } }, { c: { $ne: 100 } }] };
   var expRecs1 = [//subcl1
      { a: -101, b: -101, c: 101.01 },
      { a: -201, b: -201, c: { $decimal: "123.456" } },
      { a: -301, b: -301, c: 10000 },
      { a: -401, b: -401, c: { $date: "2017-05-02" } },
      { a: -501, b: -501, c: { $timestamp: "2017-05-02-15.32.18.000000" } },
      { a: -601, b: -601, c: { $oid: "123abcd00ef12358902300ef" } },
      { a: -701, b: -701, c: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
      { a: -801, b: -801, c: "abc" },
      { a: -901, b: -901, c: [1, 2, 3] },
      { a: -999, b: -999, c: { $minKey: 1 } },
      //subcl2
      { a: 0, b: 0, c: 200 },
      { a: 201, b: 201, c: 202.02 },
      { a: 301, b: 301, c: { $decimal: "456.789" } },
      { a: 401, b: 401, c: 20000 },
      { a: 501, b: 501, c: { $date: "2017-05-03" } },
      { a: 601, b: 601, c: { $timestamp: "2017-05-03-15.32.18.000000" } },
      { a: 701, b: 701, c: "efg" },
      { a: 801, b: 801, c: [3, 4, 5, 6] },
      { a: 901, b: 901, c: null },
      { a: 998, b: 998, c: true }, { a: 999, b: 999, c: false },
      //subcl3
      { a: 1000, b: 1000, c: 1000 },
      { a: 1001, b: 1001, c: 1001.01 },
      { a: 1200, b: 1200, c: { $decimal: "2017.05" } },
      { a: 1300, b: 1300, c: 30000 },
      { a: 1400, b: 1400, c: { $date: "2017-05-04" } },
      { a: 1500, b: 1500, c: { $timestamp: "2017-05-04-15.32.18.000000" } },
      { a: 1600, b: 1600, c: "hkj" },
      { a: 1700, b: 1700, c: [6, 10, 20] },
      { a: 1800, b: 1800, c: { name: "Jack" } },
      { a: 1999, b: 1999, c: { $maxKey: 1 } }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //mod
   var findCondition1 = { $and: [{ a: { $mod: [2, 1] } }, { b: { $mod: [2, 1] } }, { c: { $mod: [2, 1] } }] };
   var expRecs1 = [//subcl2
      { a: 801, b: 801, c: [3, 4, 5, 6] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //in
   var findCondition1 = {
      $and: [{ a: { $in: [-101, 801, 201, 998, 1001, 1700] } },
      { b: { $in: [201, 801, 1200, 1700] } },
      { c: { $in: [5, 6, 10] } }]
   };
   var expRecs1 = [//subcl2
      { a: 801, b: 801, c: [3, 4, 5, 6] },
      //subcl3
      { a: 1700, b: 1700, c: [6, 10, 20] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //all
   var findCondition1 = { $and: [{ a: { $all: [-901] } }, { b: { $all: [-901] } }, { c: { $all: [2, 3] } }] };
   var expRecs1 = [//subcl1
      { a: -901, b: -901, c: [1, 2, 3] }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //exists
   var findCondition1 = { $and: [{ a: { $exists: 0 } }, { b: { $exists: 0 } }] };
   var expRecs1 = [];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //isnull
   var findCondition1 = { $and: [{ a: { $isnull: 1 } }, { b: { $isnull: 1 } }] };
   var expRecs1 = [];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   commDropCL( db, COMMCSNAME, subCL_Name1, true, true, "clean sub collection in the end" );
   commDropCL( db, COMMCSNAME, subCL_Name2, true, true, "clean sub collection in the end" );
   commDropCL( db, COMMCSNAME, subCL_Name3, true, true, "clean sub collection in the end" );
   commDropCL( db, COMMCSNAME, mainCL_Name, true, true, "clean main collection in the end" );
}