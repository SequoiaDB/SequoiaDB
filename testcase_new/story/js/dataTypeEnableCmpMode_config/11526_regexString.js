/************************************
*@Description: set enablemixcmp=false;regex string,
               indexScan and tableScan have the same result;
*@author:      zhaoyu
*@createdate:  2017.5.18
*@testlinkCase: seqDB-11526
**************************************/
main( test );

function test ()
{
   var clName = COMMCLNAME + "_11526";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //create index
   commCreateIndex( dbcl, "a", { a: 1 } );

   var hintConf = [{ "": "a" }, { "": null }];
   var sortConf = { _id: 1 };

   //insert all type data 
   var doc = [{ a: 100 },
   { a: { $numberLong: "200" } },
   { a: 100.23 },
   { a: { $decimal: "300" } },
   { a: "aa" }, { a: "ab" }, { a: "Aa" }, { a: "Ab" }, { a: "b" },
   { a: { $oid: "591cf397a54fe50425000000" } },
   { a: true },
   { a: { $date: "2014-01-01" } },
   { a: { $timestamp: "2015-06-05-16.10.33.000000" } },
   { a: { $binary: "aGVsbG8gd29ybGQ=", $type: "1" } },
   { a: { $options: "ih", $regex: "ab" } },
   { a: { $regex: "ab", $options: "ih" } },
   { a: { $regex: "ab", $options: "ij" } },
   { a: { $regex: "ab", $options: "j" } },
   { a: { $regex: "a" } },
   { a: { $regex: "b" } },
   { a: { b: 1 } },
   { a: [1, 20, 30] },
   { a: null },
   { a: { $minKey: 1 } },
   { a: { $maxKey: 1 } },
   { b: 1 }];
   dbcl.insert( doc );

   var findConf1 = { a: { $regex: "^a", $options: "i" } };
   var expRecs1 = [{ a: "aa" }, { a: "ab" }, { a: "Aa" }, { a: "Ab" }];
   checkResult( dbcl, findConf1, hintConf, sortConf, expRecs1 );

   var findConf2 = { a: { $options: "i", $regex: "^a" } };
   var expRecs2 = [{ a: "aa" }, { a: "ab" }, { a: "Aa" }, { a: "Ab" }];
   checkResult( dbcl, findConf2, hintConf, sortConf, expRecs2 );

   var findConf3 = { a: { $regex: "^a" } };
   var expRecs3 = [{ a: "aa" }, { a: "ab" }];
   checkResult( dbcl, findConf3, hintConf, sortConf, expRecs3 );

   var findConf4 = { a: { $options: "i" } };
   InvalidArgCheck( dbcl, findConf4, null, SDB_INVALIDARG );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the end" );
}
