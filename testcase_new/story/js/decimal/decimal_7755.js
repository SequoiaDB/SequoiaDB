/************************************
*@Description: decimal data use max
*@author:      zhaoyu
*@createdate:  2016.4.29
**************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_7755";
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //insert decimal data
   var doc = [{ dep: "develop", score: { $decimal: "-9223372036854775808123" }, name: "min_lilei1" },
   { dep: "develop", score: { $decimal: "9223372036854775807456", $precision: [100, 2] }, name: "lilei2" },
   { dep: "develop", score: { $decimal: "9223372036854775807456" }, name: "max_lilei2" },
   { dep: "develop", score: { $decimal: "-4.7E-330" }, name: "lilei3" },
   { dep: "develop", score: { $decimal: "5.7E-340", $precision: [1000, 350] }, name: "lilei4" },
   { dep: "develop", score: { $decimal: "0" }, name: "lilei5" },
   { dep: "develop", score: 1204, name: "lilei6" },
   { dep: "test", score: { $decimal: "126" }, name: "max_hanmeimei1" },
   { dep: "test", score: { $decimal: "125" }, name: "min_hanmeimei2" }];
   dbcl.insert( doc );

   //check result
   condition = { $group: { _id: "$dep", max_score: { $max: "$score" } } };
   var expRecs = [{ max_score: { "$decimal": "9223372036854775807456.00", "$precision": [100, 2] } },
   { max_score: { "$decimal": "126" } }];
   aggregateCheckResult( dbcl, condition, expRecs );
   commDropCL( db, COMMCSNAME, clName );
}

