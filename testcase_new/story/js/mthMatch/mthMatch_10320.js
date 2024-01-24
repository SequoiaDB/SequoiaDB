/************************************
*@Description: function use mod,matches use mod
*@author:      zhaoyu
*@createdate:  2016.10.25
*@testlinkCase: 
**************************************/
main( test );
function test ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, COMMCLNAME );

   //create index
   commCreateIndex( dbcl, "a", { a: 1 } );

   //insert data 
   var doc = [{ No: 1, a: 125 },
   { No: 2, a: 126 }];
   dbcl.insert( doc );

   var findCondition1 = { a: { $mod: 5, $mod: [5, 0] } };
   var expRecs1 = [{ No: 1, a: 125 }];
   checkResult( dbcl, findCondition1, null, expRecs1, { _id: 1 } );
}