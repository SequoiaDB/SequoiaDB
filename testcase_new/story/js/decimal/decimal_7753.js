/************************************
*@Description: decimal data use sum
*@author:      zhaoyu
*@createdate:  2016.5.5
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_7753";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //insert data
   var doc = [{ dep: "develop", score: { $decimal: "-9223372036854775808123" }, name: "lilei1" },
   { dep: "develop", score: { $decimal: "9223372036854775807456", $precision: [100, 2] }, name: "lilei2" },
   { dep: "develop", score: { $decimal: "158.97" }, name: "lilei3" },
   { dep: "develop", score: { $decimal: "-4.7E-330" }, name: "lilei4" },
   { dep: "develop", score: { $decimal: "5.7E-340", $precision: [1000, 350] }, name: "lilei5" },
   { dep: "develop", score: { $decimal: "5.7E+310", $precision: [1000, 2] }, name: "lilei6" },
   { dep: "develop", score: { $decimal: "-9.7E+310", $precision: [1000, 2] }, name: "lilei7" },
   { dep: "develop", score: { $decimal: "0" }, name: "lilei8" },
   { dep: "develop", score: 1204, name: "lilei9" },
   { dep: "test", score: { $decimal: "126" }, name: "hanmeimei1" },
   { dep: "test", score: { $numberLong: "125" }, name: "hanmeimei2" },
   { dep: "test", score: "string", name: "hanmeimei3" }];
   dbcl.insert( doc );

   //check result
   condition = { $group: { _id: "$dep", sum_score: { $sum: "$score" } } };
   var expRecs = [{ sum_score: { "$decimal": "-39999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999304.03000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000469999999943000000000" } },
   { sum_score: { "$decimal": "251" } }];
   aggregateCheckResult( dbcl, condition, expRecs );
   commDropCL( db, COMMCSNAME, clName );
}

