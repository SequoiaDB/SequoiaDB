/************************************
*@Description: use floor to match,
               type is arr/obj,value set typical data;
               use arr/obj name or arr/obj element to compare
*@author:      zhaoyu
*@createdate:  2016.10.13
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
   var doc = [{ No: 1, a: [-1.2, -2.5], b: [1.2, 2.5] },
   { No: 2, a: [{ 0: -1.2 }, { 1: -2.5 }], b: [{ 0: 1.2 }, { 1: 2.5 }] },
   { No: 3, a: { 0: -1.2, 1: -2.5 }, b: { 0: 1.2, 1: 2.5 } },
   { No: 4, a: [-2.2, -3.5], b: [2.2, 3.5] },
   { No: 5, a: [{ 0: -2.2 }, { 1: -3.5 }], b: [{ 0: 2.2 }, { 1: 3.5 }] },
   { No: 6, a: { 0: -2.2, 1: -3.5 }, b: { 0: 2.2, 1: 3.5 } }];
   dbcl.insert( doc );

   //arr or obj name query
   var findCondition1 = { $and: [{ a: { $floor: 1, $et: -2 } }, { b: { $floor: 1, $et: 2 } }] };
   var expRecs1 = [{ No: 1, a: [-1.2, -2.5], b: [1.2, 2.5] }];
   checkResult( dbcl, findCondition1, null, expRecs1, { No: 1 } );

   //arr or obj element query
   var findCondition2 = { $and: [{ "a.0": { $floor: 1, $et: -2 } }, { "b.1": { $floor: 1, $et: 2 } }] };
   var expRecs2 = [{ No: 1, a: [-1.2, -2.5], b: [1.2, 2.5] },
   { No: 2, a: [{ 0: -1.2 }, { 1: -2.5 }], b: [{ 0: 1.2 }, { 1: 2.5 }] },
   { No: 3, a: { 0: -1.2, 1: -2.5 }, b: { 0: 1.2, 1: 2.5 } }];
   checkResult( dbcl, findCondition2, null, expRecs2, { No: 1 } );
}
