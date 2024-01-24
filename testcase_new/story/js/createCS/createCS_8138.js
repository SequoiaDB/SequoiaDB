//creat same collection space

main( test );
function test ()
{
   var csName = COMMCSNAME + "_8138";

   commDropCS( db, csName );
   db.createCS( csName );

   assert.tryThrow( SDB_DMS_CS_EXIST, function()
   {
      db.createCS( csName );
   } );
   commDropCS( db, csName );
}