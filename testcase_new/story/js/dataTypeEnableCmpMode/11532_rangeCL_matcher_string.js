/************************************
*@Description: subcl query, index scan and table scan
*@author:      zhaoyu
*@createdate:  2017.5.20
*@testlinkCase:seqDB-11532
**************************************/
main( test );
function test ()
{
   //set find data from master
   db.setSessionAttr( { PreferedInstance: "M" } );

   var clName = COMMCLNAME + "_11532";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

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
   var doc = [{ a: -10 },
   { a: { $date: "2017-05-01" } },
   { a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: true },
   { a: { name: "Jack" } },
   { b: 1 },
   { a: "aa" },
   { a: "ab" },
   { a: "abc" },
   { a: "ac" },
   { a: "ad" },
   { a: "ae" },
   { a: "ba" },
   { a: ["aa"] },
   { a: ["ab"] },
   { a: ["abc"] },
   { a: ["ac"] },
   { a: ["ad"] },
   { a: ["ae"] },
   { a: ["ba"] }];
   dbcl.insert( doc );

   //split cl
   var startCondition1 = 50;
   ClSplitOneTimes( COMMCSNAME, clName, startCondition1, null );

   //gt
   var findConf1 = { a: { $gt: "ab" } };
   var expRecs1 = [{ a: "abc" },
   { a: "ac" },
   { a: "ad" },
   { a: "ae" },
   { a: "ba" },
   { a: ["abc"] },
   { a: ["ac"] },
   { a: ["ad"] },
   { a: ["ae"] },
   { a: ["ba"] }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   //gte
   var findConf2 = { a: { $gte: "ab" } };
   var expRecs2 = [{ a: "ab" },
   { a: "abc" },
   { a: "ac" },
   { a: "ad" },
   { a: "ae" },
   { a: "ba" },
   { a: ["ab"] },
   { a: ["abc"] },
   { a: ["ac"] },
   { a: ["ad"] },
   { a: ["ae"] },
   { a: ["ba"] }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   //lt
   var findConf3 = { a: { $lt: "ae" } };
   var expRecs3 = [{ a: "aa" },
   { a: "ab" },
   { a: "abc" },
   { a: "ac" },
   { a: "ad" },
   { a: ["aa"] },
   { a: ["ab"] },
   { a: ["abc"] },
   { a: ["ac"] },
   { a: ["ad"] }];
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   //lte
   var findConf4 = { a: { $lte: "ad" } };
   var expRecs4 = [{ a: "aa" },
   { a: "ab" },
   { a: "abc" },
   { a: "ac" },
   { a: "ad" },
   { a: ["aa"] },
   { a: ["ab"] },
   { a: ["abc"] },
   { a: ["ac"] },
   { a: ["ad"] }];
   checkResult( dbcl, findConf4, hintConf, sortConf, expRecs4 );

   //et
   var findConf5 = { a: { $et: "ad" } };
   var expRecs5 = [{ a: "ad" },
   { a: ["ad"] }];
   checkResult( dbcl, findConf5, hintConf, sortConf, expRecs5 );

   //ne
   var findConf6 = { a: { $ne: "ad" } };
   var expRecs6 = [{ a: -10 },
   { a: { $date: "2017-05-01" } },
   { a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: true },
   { a: { name: "Jack" } },
   { a: "aa" },
   { a: "ab" },
   { a: "abc" },
   { a: "ac" },
   { a: "ae" },
   { a: "ba" },
   { a: ["aa"] },
   { a: ["ab"] },
   { a: ["abc"] },
   { a: ["ac"] },
   { a: ["ae"] },
   { a: ["ba"] }];
   checkResult( dbcl, findConf6, hintConf, sortConf, expRecs6 );

   //in
   var findConf7 = { a: { $in: ["aa", "ad", "ba", "ya", "yf"] } };
   var expRecs7 = [{ a: "aa" },
   { a: "ad" },
   { a: "ba" },
   { a: ["aa"] },
   { a: ["ad"] },
   { a: ["ba"] }];
   checkResult( dbcl, findConf7, hintConf, sortConf, expRecs7 );

   //all
   var findConf8 = { a: { $all: ["aa"] } };
   var expRecs8 = [{ a: "aa" },
   { a: ["aa"] }];
   checkResult( dbcl, findConf8, hintConf, sortConf, expRecs8 );

   //{$regex}, SEQUOIADBMAINSTREAM-2458
   var findConf9 = { a: { $regex: "^a" } };
   var expRecs9 = [{ a: "aa" },
   { a: "ab" },
   { a: "abc" },
   { a: "ac" },
   { a: "ad" },
   { a: "ae" },
   { a: ["aa"] },
   { a: ["ab"] },
   { a: ["abc"] },
   { a: ["ac"] },
   { a: ["ad"] },
   { a: ["ae"] }];
   checkResult( dbcl, findConf9, hintConf, sortConf, expRecs9 );

   var findConf10 = { a: { $regex: "^a", $options: "i" } };
   var expRecs10 = [{ a: "aa" },
   { a: "ab" },
   { a: "abc" },
   { a: "ac" },
   { a: "ad" },
   { a: "ae" },
   { a: ["aa"] },
   { a: ["ab"] },
   { a: ["abc"] },
   { a: ["ac"] },
   { a: ["ad"] },
   { a: ["ae"] }];
   checkResult( dbcl, findConf10, hintConf, sortConf, expRecs10 );

   var findConf11 = { a: { $options: "i", $regex: "^a" } };
   var expRecs11 = [{ a: "aa" },
   { a: "ab" },
   { a: "abc" },
   { a: "ac" },
   { a: "ad" },
   { a: "ae" },
   { a: ["aa"] },
   { a: ["ab"] },
   { a: ["abc"] },
   { a: ["ac"] },
   { a: ["ad"] },
   { a: ["ae"] }];
   checkResult( dbcl, findConf11, hintConf, sortConf, expRecs11 );

   var findConf12 = { a: { $options: "i" } };
   InvalidArgCheck( dbcl, findConf12, null, SDB_INVALIDARG );

   var findConf11 = { a: { $options: "i", $regex: "^a", $lt: "ae" } };
   var expRecs11 = [{ a: "aa" },
   { a: "ab" },
   { a: "abc" },
   { a: "ac" },
   { a: "ad" },
   { a: ["aa"] },
   { a: ["ab"] },
   { a: ["abc"] },
   { a: ["ac"] },
   { a: ["ad"] }];
   checkResult( dbcl, findConf11, hintConf, sortConf, expRecs11 );
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}
