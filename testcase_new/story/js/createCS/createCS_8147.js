// list cs_2.

main( test );
function test ()
{
   var csName = COMMCSNAME + "_8147";
   var csNames = [];

   for( i = 0; i < 100; i++ )
   {
      csNames.push( csName + i );
      commDropCS( db, csNames[i] );
      db.createCS( csNames[i] );
   }

   db.dropCS( csNames[0] );
   db.dropCS( csNames[1] );

   var reserdCSNum = 0;
   var cur = db.listCollectionSpaces();
   while( cur.next() )
   {
      var actName = cur.current().toObj()["Name"];
      for( var j = 0; j < csNames.length; j++ )
      {
         if( actName == csNames[j] )
         {
            reserdCSNum++;
            break;
         }
      }

      if( actName == csNames[0] || actName == csNames[1] )
      {
         throw new Error( csNames[0] + " or " + csNames[1] + " should't exists" );
      }
   }

   assert.equal( reserdCSNum, csNames.length - 2 );

   for( i = 2; i < 100; i++ )
   {
      commDropCS( db, csNames[i] );
   }
}