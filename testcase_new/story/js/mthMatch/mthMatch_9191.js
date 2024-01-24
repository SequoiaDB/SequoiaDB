/************************************
*@Description: find condition type is obj,key exists $ and feildName type is other 
*@author:      zhaoyu
*@createdate:  2016.10.24
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
   var doc = [{ No: 1, a: { b: 1 }, b: { c: 1 } },
   { No: 2, a: { b: 2 }, b: { c: 2 } },
   { No: 3, a: { b: 3 }, b: { c: 3 } },
   { No: 4, a: { b: 4 }, b: { c: 4 } }];
   dbcl.insert( doc );

   //seqDB-9191
   var findCondition1 = { a: { $exists: 1, b: 1 } };
   InvalidArgCheck( dbcl, findCondition1, null, -6 );

   var findCondition2 = { b: { $exists: 1, c: 1 } };
   InvalidArgCheck( dbcl, findCondition2, null, -6 );

   //feildName exists $ or .
   dbcl.remove();
   //insert data 
   var doc = [{ No: 1, a: [{ b: 1 }, { b: 2 }], b: [{ c: 1 }] },
   { No: 2, a: [{ b: 2 }, { b: 3 }], b: [{ c: 2 }] },
   { No: 3, a: [{ b: 3 }, { b: 4 }], b: [{ c: 3 }] },
   { No: 4, a: [{ b: 4 }, { b: 5 }], b: [{ c: 4 }] }];
   dbcl.insert( doc );

   //seqDB-9191
   var findCondition1 = { "a.$0": { $exists: 1, b: 1 } };
   InvalidArgCheck( dbcl, findCondition1, null, -6 );

   var findCondition2 = { "b.$0": { $exists: 1, c: 1 } };
   InvalidArgCheck( dbcl, findCondition2, null, -6 );

   var findCondition3 = { "a.0": { $exists: 1, b: 1 } };
   InvalidArgCheck( dbcl, findCondition3, null, -6 );

   var findCondition4 = { "b.0": { $exists: 1, c: 1 } };
   InvalidArgCheck( dbcl, findCondition4, null, -6 );
}
