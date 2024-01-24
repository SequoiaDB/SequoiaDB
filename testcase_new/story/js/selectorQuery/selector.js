// query record.
// normal selector

main( test );
function test ()
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );

   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS in the beginning" );
   var varCL = varCS.createCL( COMMCLNAME, { ReplSize: 0, Compressed: true } );

   varCL.insert( { name: "Tom", age: 15 } );
   varCL.insert( { name: "John" } );
   varCL.insert( { name: "tom", address: { street: { street1: "1024 Wall Street", street2: "University Drive" }, zipcode: 100000 } } );
   varCL.insert( { phone: ["123", "456"] } );
   var rc = varCL.find();

   var size = 0;
   while( true )
   {
      var i = rc.next();
      if( !i )
         break;
      else
         size++;
   }
   assert.equal( 4, size );

   var rc1 = varCL.find( null, { name: null } );


   var size1 = 0;
   while( true )
   {
      var i = rc1.next();
      if( !i )
         break;
      else
         size1++;
   }
   assert.equal( 4, size );

   var rc2 = varCL.find( { name: { $exists: 1 } }, { name: null } );


   var size2 = 0;
   while( true )
   {
      var i = rc2.next();
      if( !i )
         break;
      else
         size2++;
   }
   assert.equal( 3, size2 );

   var rc3 = varCL.find( { "phone.0": { $exists: 1 } }, { "phone.0": "78" } );

   var size3 = 0;
   while( true )
   {
      var i = rc3.next();
      if( !i )
         break;
      else
         size3++;
   }
   assert.equal( 1, size3 );

   var rc4 = varCL.find( { "phone.0": { $exists: 0 } }, { "phone.0": "78" } );

   var size4 = 0;
   while( true )
   {
      var i = rc4.next();
      if( !i )
         break;
      else
         size4++;
   }
   assert.equal( 3, size4 );
}
