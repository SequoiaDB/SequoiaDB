// upsert record.
// normal case.

main( test );
function test ()
{
   // clear
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true, "drop cl in the beginning" );

   // create cs, cl
   var varCL = commCreateCL( db, COMMCSNAME, COMMCLNAME, {}, true, false );

   varCL.insert( { a: 1 } );

   varCL.upsert( { $set: { a: 2 } }, { a: 1 } );

   checkResult( varCL, {}, [{ a: 2 }] );

   varCL.upsert( { $set: { a: 4 } }, { a: 3 } );

   checkResult( varCL, {}, [{ a: 2 }, { a: 4 }] );
   //zhaoyu add
   varCL.upsert( { $set: { a: 5 } }, { $or: [{ b: 1 }] } );

   checkResult( varCL, {}, [{ a: 2 }, { a: 4 }, { a: 5, b: 1 }] );

   // clear end
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" );
}