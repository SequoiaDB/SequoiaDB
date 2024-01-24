/* *****************************************************************************
@discretion: test alter clname
@author��2018-4-16 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_alterclName14934";

main( test );
function test ()
{

   //clean environment before test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //alter clName
   assert.tryThrow( SDB_OPTION_NOT_SUPPORT, function()
   {
      dbcl.setAttributes( { "Name": COMMCSNAME + "." + clName } );
   } );

   //clean
   commDropCL( db, COMMCSNAME, clName, true, true );
}