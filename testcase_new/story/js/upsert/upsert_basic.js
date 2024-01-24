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

   var rc = varCL.find( { a: 2 } );

   var size = rc.size();
   assert.equal( 1, size );

   varCL.upsert( { $set: { a: 4 } }, { a: 3 } );

   rc = varCL.find();

   size = 0;

   var bAddNew = false;
   while( rc.next() )
   {
      var recordObj = rc.current().toObj();
      var recordStr = rc.current().toJson();
      if( recordObj["a"] == 4 )
      {
         bAddNew = true;
      }
      size++;
   }

   if( size != 2 || !bAddNew )
   {
      throw new Error( "size!=2" );
   }

   //zhaoyu add
   varCL.upsert( { $set: { a: 5 } }, { $or: [{ b: 1 }] } );

   rc = varCL.find();

   size = 0;

   var bAddNew = false;
   while( rc.next() )
   {
      var recordObj = rc.current().toObj();
      var recordStr = rc.current().toJson();
      if( recordObj["a"] == 5 && recordObj["b"] === 1 )
      {
         bAddNew = true;
      }
      size++;
   }

   if( size != 3 || !bAddNew )
   {
      throw new Error( "size!=3" );
   }

   // clear end
   commDropCL( db, COMMCSNAME, COMMCLNAME, false, false, "drop cl in the end" );

}