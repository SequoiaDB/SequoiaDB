//creat cl
//innomal case2

main( test );
function test ()
{
   var clName = "." + "_8159";
   var cs = db.getCS( COMMCSNAME );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cs.createCL( clName );
   } );
}