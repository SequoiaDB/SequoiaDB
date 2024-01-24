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
   { a: { $date: "2017-05-01" } },
   { a: { $timestamp: "2015-05-01-15.32.18.000000" } },
   { a: { $timestamp: "2016-05-01-15.32.18.000000" } },
   { a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { a: { $timestamp: "2018-05-01-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "abc" },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: true },
   { a: { name: "Jack" } },
   { b: 1 }];
   dbcl.insert( doc );

   //split cl
   var startCondition1 = { a: { $timestamp: "2017-05-01-15.32.18.000000" } };
   ClSplitOneTimes( COMMCSNAME, clName, startCondition1, null );

   //$et
   var findCondition1 = { a: { $et: { $timestamp: "2017-05-01-15.32.18.000000" } } };
   var expRecs1 = [{ a: { $timestamp: "2017-05-01-15.32.18.000000" } }];
   checkResult( dbcl, findCondition1, hintConf, sortConf, expRecs1 );

   //$gt
   var findCondition2 = { a: { $gt: { $timestamp: "2015-05-01-15.32.18.000000" } } };
   var expRecs2 = [{ a: { $date: "2017-05-01" } },
   { a: { $timestamp: "2016-05-01-15.32.18.000000" } },
   { a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { a: { $timestamp: "2018-05-01-15.32.18.000000" } }];
   checkResult( dbcl, findCondition2, hintConf, sortConf, expRecs2 );

   //gte
   var findCondition3 = { a: { $gte: { $timestamp: "2016-05-01-15.32.18.000000" } } };
   var expRecs3 = [{ a: { $date: "2017-05-01" } },
   { a: { $timestamp: "2016-05-01-15.32.18.000000" } },
   { a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { a: { $timestamp: "2018-05-01-15.32.18.000000" } }];
   checkResult( dbcl, findCondition3, hintConf, sortConf, expRecs3 );

   //lt
   var findCondition4 = { a: { $lt: { $timestamp: "2018-05-01-15.32.18.000000" } } };
   var expRecs4 = [{ a: { $date: "2017-05-01" } },
   { a: { $timestamp: "2015-05-01-15.32.18.000000" } },
   { a: { $timestamp: "2016-05-01-15.32.18.000000" } },
   { a: { $timestamp: "2017-05-01-15.32.18.000000" } }];
   checkResult( dbcl, findCondition4, hintConf, sortConf, expRecs4 );

   //lte
   var findCondition5 = { a: { $lte: { $timestamp: "2017-05-01-15.32.18.000000" } } };
   var expRecs5 = [{ a: { $date: "2017-05-01" } },
   { a: { $timestamp: "2015-05-01-15.32.18.000000" } },
   { a: { $timestamp: "2016-05-01-15.32.18.000000" } },
   { a: { $timestamp: "2017-05-01-15.32.18.000000" } }];
   checkResult( dbcl, findCondition5, hintConf, sortConf, expRecs5 );

   //ne
   var findCondition6 = { a: { $ne: { $timestamp: "2015-05-01-15.32.18.000000" } } };
   var expRecs6 = [{ a: 10 },
   { a: { $date: "2017-05-01" } },
   { a: { $timestamp: "2016-05-01-15.32.18.000000" } },
   { a: { $timestamp: "2017-05-01-15.32.18.000000" } },
   { a: { $timestamp: "2018-05-01-15.32.18.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $regex: "^z", $options: "i" } },
   { a: null },
   { a: { $oid: "123abcd00ef12358902300ef" } },
   { a: "abc" },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { a: true },
   { a: { name: "Jack" } }];
   checkResult( dbcl, findCondition6, hintConf, sortConf, expRecs6 );

   //in
   var findConf8 = {
      a: {
         $in: [{ $timestamp: "2015-05-01-15.32.18.000000" },
         { $timestamp: "2018-05-01-15.32.18.000000" },
         { $date: "2017-11-01" },
         { $date: "2017-05-01" },
         { $date: "2017-05-03" }]
      }
   };
   var expRecs8 = [{ a: { $date: "2017-05-01" } },
   { a: { $timestamp: "2015-05-01-15.32.18.000000" } },
   { a: { $timestamp: "2018-05-01-15.32.18.000000" } }];
   checkResult( dbcl, findConf8, hintConf, sortConf, expRecs8 );

   //all
   var findConf9 = { a: { $all: [{ $timestamp: "2015-05-01-15.32.18.000000" }] } };
   var expRecs9 = [{ a: { $timestamp: "2015-05-01-15.32.18.000000" } }];
   checkResult( dbcl, findConf9, hintConf, sortConf, expRecs9 );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}