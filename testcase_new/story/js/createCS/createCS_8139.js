// create cs.
// unnormal_1 case.

main( test );
function test ()
{
   var csName = "$" + COMMCSNAME + "_8139";

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createCS( csName );
   } );
}