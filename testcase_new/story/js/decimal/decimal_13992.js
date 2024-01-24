/******************************************************************************
*@Description : test remove special decimal value
*               seqDB-13992:删除特殊decimal值             
*@author      : Liang XueWang 
******************************************************************************/
main( test )
function test ()
{
   var clName = COMMCLNAME + "_13992";
   var docs = [{ a: { $decimal: "MAX" } },
   { a: { $decimal: "MIN" } },
   { a: { $decimal: "NaN" } }];

   commDropCL( db, COMMCSNAME, clName );
   var cl = commCreateCL( db, COMMCSNAME, clName );
   cl.insert( docs );

   for( var i = 0; i < docs.length; i++ )
   {
      cl.remove( docs[i] );
      var docNum = cl.count( docs[i] );
      if( parseInt( docNum ) !== 0 )
      {
         throw new Error( "parseInt( docNum ): " + parseInt( docNum ) );
      }
   }
   commDropCL( db, COMMCSNAME, clName );
}
