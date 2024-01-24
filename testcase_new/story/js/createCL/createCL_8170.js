// create cl.
// normal case.
testConf.csName = COMMCSNAME + "_8170";
main( test );
function test ( args )
{
   var clName = CHANGEDPREFIX + "_8170";
   var cs = args.testCS;
   var aa = Array( ";", ":", "\'", "\"", "{", "}", "[", "]", ",", "<", ">", "?", "/", "|", "\\", "+", "=", "-", "_", "~", "`", "!", "@", "#", "$", "%", "^", "&", "*", "(", ")" );
   for( var i = 0; i < aa.length; i++ )
   {
      var actClName = clName + aa[i];
      cs.createCL( actClName );
   }

   for( var i = 0; i < aa.length; i++ )
   {
      var actClName = clName + aa[i];
      var cl = cs.getCL( actClName );
      cl.insert( { a: 1 } );
   }
}
