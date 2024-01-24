//creat cl
//innomal case3

main( test );
function test ()
{
   var clName = "";
   var cs = db.getCS( COMMCSNAME );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cs.createCL( clName );
   } );
}