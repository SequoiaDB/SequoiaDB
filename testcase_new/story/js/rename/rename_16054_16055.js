/* *****************************************************************************
@discretion: rename cl, test case 16054/16055:
             testcase-16054:the old cl is not exist
             testcase-16055:the new cl is exist
@author��2018-10-12 wuyan  Init
***************************************************************************** */

main( test );
function test ()
{
   var clName = CHANGEDPREFIX + "_renamecl16054";
   var newCLName = CHANGEDPREFIX + "_newcl16054";
   var clName1 = CHANGEDPREFIX + "_renamecl16055";
   var newCLName = CHANGEDPREFIX + "_newcl16054";
   commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the beginning" );
   commDropCL( db, COMMCSNAME, clName1, true, true, "clear collection in the beginning" );
   var dbcl = commCreateCL( db, COMMCSNAME, clName1, { ReplSize: 0 }, true, true );
   commCreateCL( db, COMMCSNAME, newCLName );

   //test case-16054: the old cl is not exist
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      db.getCS( COMMCSNAME ).renameCL( clName, newCLName );
   } );

   //test case-16055: the new cl is exist
   assert.tryThrow( SDB_DMS_EXIST, function()
   {
      db.getCS( COMMCSNAME ).renameCL( clName1, newCLName );
   } );

   commDropCL( db, COMMCSNAME, newCLName, true, true, "clear collection in the ending" );
}