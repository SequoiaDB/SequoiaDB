//create exist collection case3
TESTCSNAME_1 = CHANGEDPREFIX + "foo_1";
TESTCSNAME_2 = CHANGEDPREFIX + "foo_2";
TESTCLNAME = CHANGEDPREFIX + "bar";

main( test );
function test ()
{
   var csName1 = COMMCSNAME + "_8164_1";
   var csName2 = COMMCSNAME + "_8164_2";
   var clName = COMMCLNAME + "_8164";
   commDropCS( db, csName1 );
   commDropCS( db, csName2 );

   var cs1 = commCreateCS( db, csName1 );
   var cs2 = commCreateCS( db, csName2 );

   cs1.createCL( clName, { Compressed: true } );
   cs2.createCL( clName, { Compressed: true } );

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
}