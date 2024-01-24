/******************************************************************************
@Description : 1.createIndex basic
@Modify list :
               2015-01-23  pusheng Ding  Init
******************************************************************************/
CHANGEDPREFIX_IND = CHANGEDPREFIX + "_7395_ind";
CHANGEDPREFIX_IND1 = CHANGEDPREFIX + "_7395_ind1";

main( test );
function test ()
{
   db.setSessionAttr( { PreferedInstance: "M" } );
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );
   var varCS = commCreateCS( db, COMMCSNAME, true, "create CS" );
   var varCL = varCS.createCL( COMMCLNAME, { ReplSize: 1 } );

   varCL.createIndex( CHANGEDPREFIX_IND, { a: 1 } );
   for( var i = 0; i < 100; i++ )
   {
      varCL.insert( { a: 100 - i, b: i, c: "abcdefghijkl" + i } );
   }

   varCL.createIndex( CHANGEDPREFIX_IND1, { b: 1, a: 1 }, true, false );

   var sel = varCL.listIndexes();
   var size = 0;
   var exprows = 3;
   var flag = true;
   while( sel.next() )
   {
      size++;
      if( size > exprows )
      {
         flag = false;
         throw new Error( "list-indexes-incorrect" );
      }
      var ret = sel.current();
   }
   sel.close();
   if( flag && size != exprows )
   {
      flag = false;
      throw new Error( "list-indexes-incorrect" );
   }
   varCL.dropIndex( CHANGEDPREFIX_IND );
   varCL.dropIndex( CHANGEDPREFIX_IND1 );

   var sel = varCL.listIndexes();
   var size = 0;
   var exprows = 1;
   var flag = true;
   while( sel.next() )
   {
      size++;
      if( size > exprows )
      {
         flag = false;
         throw new Error( "list-indexes-incorrect" );
      }
      var ret = sel.current();
   }
   sel.close();
   if( flag && size != exprows )
   {
      flag = false;
      throw new Error( "list-indexes-incorrect" );
   }

   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true );
}
