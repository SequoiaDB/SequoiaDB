// create cs.
// unnormal_3 case
main( test );
function test ()
{
   var csName = "";

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createCS( csName );
   } );
}