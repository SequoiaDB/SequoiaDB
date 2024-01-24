/******************************************************************************
*@Description : 1.SEQUOIADBMAINSTREAM-272
*               fixed: SVN 13586
*@Modify list :
*               2014-07-17 pusheng Ding  Init
******************************************************************************/

main( test );

function test ()
{
   indexName = CHANGEDPREFIX + "idx";
   commDropCL( db, csName, clName );
   var varCL = commCreateCL( db, csName, clName );

   varCL.createIndex( indexName, { a: 1 } );

   var docs = [];
   for( var i = 0; i < 2621 * 5; i += 5 )
   {
      docs.push( { a: i } );
   }
   varCL.insert( docs );


   var sel = varCL.find( { a: { $ne: 11780 } } );
   var size = 0;
   var flag = true;
   while( sel.next() )
   {
      size++;
      if( size > 2621 )
      {
         flag = false;
         throw new Error( "error" );
      }
      var ret = sel.current();
      if( ret.toObj()['a'] == 11780 )
      {
         flag = false;
         throw new Error( "act:" + ret.toObj()['a'] + "\nexp:" + 11780 );
      }
   }
   sel.close();
   if( flag && size != 2620 )
   {
      flag = false;
      throw new Error( "return rows not expected! expected:2620 return:" + size + ( size > 2621 ? " or more" : "" ) );
   }

   varCL.dropIndex( indexName );
   commDropCL( db, csName, clName, false, false );
}
