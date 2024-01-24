// create cl.
// normal case.

main( test );
function test ()
{
   var clName = CHANGEDPREFIX + "_8169";
   var cs = db.getCS( COMMCSNAME );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cs.createCL( "SYS" + clName );
   } );

}