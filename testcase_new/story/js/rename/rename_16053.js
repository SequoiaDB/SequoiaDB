/* *****************************************************************************
@discretion: rename cl, the new cl name is the same as the old cl name
@author��2018-10-12 wuyan  Init
***************************************************************************** */

main( test );
function test ()
{
   //create cl 
   var clName = CHANGEDPREFIX + "_renamecl16053";
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ReplSize: 0 }, true, true );

   //rename cl
   assert.tryThrow( SDB_DMS_EXIST, function()
   {
      db.getCS( COMMCSNAME ).renameCL( clName, clName );
   } );

   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the ending" );

}