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
   var doc = [{ No: 1, a: [1, 2, 3, 4, 5, 6] },
   { No: 2, a: [1, 2, 3, 12, 13, 14] }];
    dbcl.insert( doc );

   //check result
   var findCondition1 = { a: { $slice: [3, 3], $returnMatch: 0, $nin: [12, 13, 14] } };
   var expRecs1 = [{ No: 1, a: [4, 5, 6] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   var findCondition2 = { a: { $returnMatch: 0, $slice: [3, 3], $expand: 1, $nin: [12, 13, 14] } };
   var expRecs2 = [{ No: 1, a: 4 },
   { No: 1, a: 5 },
   { No: 1, a: 6 }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );
}
