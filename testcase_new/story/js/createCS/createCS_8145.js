// create cs.
// CSname's large is 127.
main( test );
function test ()
{
   var csName = COMMCSNAME + "_8145";

   var aa = Array( "; ", "\'", "{", "}", "[", "]", ", ", "+", "=", "-", "_", "~", "`", "!", "@", "#", "$", "%", "^", "&", "( ", " )" );
   for( var i = 0; i < aa.length; ++i )
   {
      var actCsName = csName + aa[i];
      commDropCS( db, actCsName );
      db.createCS( actCsName );
      commDropCS( db, actCsName );
   }
}