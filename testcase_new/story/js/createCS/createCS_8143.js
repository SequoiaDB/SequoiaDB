// create cs.
// CSname's large is 127.
main( test );
function test ()
{
   var csName = COMMCSNAME + "_8143";

   var len = csName.length;
   for( var i = 0; i < 127 - len; i++ )
   {
      csName += "a";
   }

   commDropCS( db, csName );

   db.createCS( csName );

   commDropCS( db, csName );
}