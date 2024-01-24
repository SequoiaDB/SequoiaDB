//creat cl
//innomal case4

main( test );
function test ()
{
   //128B
   var toolongCLName = "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890test4539";
   var cs = db.getCS( COMMCSNAME );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cs.createCL( toolongCLName );
   } );
}