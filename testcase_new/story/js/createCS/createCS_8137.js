// create cs.
// normal case.

main( test );
function test ()
{
   var csName = COMMCSNAME + "_8137";

   for( var i = 0; i < 10; i++ )
   {
      commDropCS( db, csName );
      db.createCS( csName );
      db.getCS( csName );
      commDropCS( db, csName );
   }
}