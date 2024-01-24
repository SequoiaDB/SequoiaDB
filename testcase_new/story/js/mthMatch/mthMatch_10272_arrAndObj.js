/************************************
*@Description: use mod to match,
               type is arr/obj,value set typical data;
               use arr/obj name or arr/obj element to compare
*@author:      zhaoyu
*@createdate:  2016.10.14
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
   var doc = [{ No: 1, a: [-10, -12, -13] },
   { No: 2, a: [10, 12, 13] },
   { No: 3, a: [{ 0: -10 }, { 1: -12 }, { 2: -13 }] },
   { No: 4, a: [{ 0: 10 }, { 1: 12 }, { 2: 13 }] },
   { No: 5, a: { 0: -10, 1: -12, 2: -13 } },
   { No: 6, a: { 0: 10, 1: 12, 2: 13 } }];
   dbcl.insert( doc );

   //arr or obj name query
   var findCondition1 = { a: { $mod: 2, $et: 1 } };
   var expRecs1 = [{ No: 2, a: [10, 12, 13] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   //arr or obj element query
   var findCondition2 = { "a.1": { $mod: -5, $et: -2 } };
   var expRecs2 = [{ No: 1, a: [-10, -12, -13] },
   { No: 3, a: [{ 0: -10 }, { 1: -12 }, { 2: -13 }] },
   { No: 5, a: { 0: -10, 1: -12, 2: -13 } }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );
}
