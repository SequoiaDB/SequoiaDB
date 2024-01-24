//create exist collection case1

main( test );
function test ()
{
   var clName = COMMCLNAME + "_8162";
   var cs = db.getCS( COMMCSNAME );

   commDropCL( db, COMMCSNAME, clName );
   cs.createCL( clName, { Compressed: true } );
   assert.tryThrow( SDB_DMS_EXIST, function()
   {
      cs.createCL( clName, { Compressed: true } );
   } );

   commDropCL( db, COMMCSNAME, clName );
}