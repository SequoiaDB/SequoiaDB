////create exist collection case2
main( test );
function test ()
{
   var clName = COMMCLNAME + "_8163";
   var cs = db.getCS( COMMCSNAME );
   cs.createCL( clName, { Compressed: true } );
   cs.dropCL( clName );
   cs.createCL( clName, { Compressed: true } );
   cs.dropCL( clName );
}