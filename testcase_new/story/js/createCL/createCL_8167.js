// create cl.
// normal case.

main( test );
function test ()
{
   var clName = CHANGEDPREFIX + "_8167";
   var cs = db.getCS( COMMCSNAME );

   var len = clName.length;
   for( var i = 0; i < ( 127 - len ); ++i )
   {
      clName += "a";
   }

   commDropCL( db, COMMCSNAME, clName );

   var cl = cs.createCL( clName );

   cs.getCL( clName );

   cl.insert( { a: 1 } );

   commDropCL( db, COMMCSNAME, clName );
}