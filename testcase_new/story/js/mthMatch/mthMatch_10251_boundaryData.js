/************************************
*@Description: use floor to match a field,
               value set boundary of double
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

   //create index
   commCreateIndex( dbcl, "a", { a: 1 } );

   //insert double data
   dbcl.remove();

   var doc1 = [{ No: 1, a: -4.9E-324 },
   { No: 2, a: -4.9E-325 },
   { No: 3, a: 4.9E-324 },
   { No: 4, a: 4.9E-325 }];
   dbcl.insert( doc1 );

   var findCondition1 = { $or: [{ a: { $floor: 1, $et: -1 } }, { a: { $floor: 1, $et: 0 } }] };
   var expRecs1 = [{ No: 1, a: -4.9E-324 },
   { No: 2, a: -4.9E-325 },
   { No: 3, a: 4.9E-324 },
   { No: 4, a: 4.9E-325 }];
   checkResult( dbcl, findCondition1, null, expRecs1, { _id: 1 } );
}
