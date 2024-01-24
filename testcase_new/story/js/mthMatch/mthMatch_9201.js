/************************************
*@Description: find condition include function,arrfunction and matches; 
*@author:      zhaoyu
*@createdate:  2016.11.7
*@testlinkCase: 
**************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //insert data 
   var doc = [{ No: 1, a: "String" },
   { No: 2, a: ["String", "STRING", "string"] }];
   dbcl.insert( doc );

   //check result
   var findCondition1 = { a: { $lower: 1, $returnMatch: 0, $et: "string" } };
   var expRecs1 = [{ No: 1, a: "String" },
   { No: 2, a: ["String", "STRING", "string"] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var findCondition2 = { a: { $lower: 1, $expand: 1, $et: "string" } };
   var expRecs2 = [{ No: 1, a: "String" },
   { No: 2, a: "String" },
   { No: 2, a: "STRING" },
   { No: 2, a: "string" }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );
}
