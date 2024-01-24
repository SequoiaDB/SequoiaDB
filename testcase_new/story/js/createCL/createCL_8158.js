//creat cl
//innomal case1

main( test );
function test ()
{
   var clName = "$" + "_8158";
   var cs = db.getCS( COMMCSNAME );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cs.createCL( clName );
   } );
}