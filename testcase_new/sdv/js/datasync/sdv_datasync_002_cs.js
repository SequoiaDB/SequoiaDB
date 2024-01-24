/****************************************************************
*@Description: createCS, check meta data about of CollectionSpace
               dropCS,check meta data about of CollectionSpace
*@author:     wangwenjing
*****************************************************************/

function buildCond ( obj, name )
{
   var cond = {}
   cond.Name = name;
   for( elem in obj )
   {
      cond[elem] = obj[elem];
   }

   return cond;
}
function main ()
{
   try
   {
      var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      if( commIsStandalone( db ) )
      {
         return;
      }

      var mgr = new groupMgr( db );
      mgr.init();

      var group = mgr.getGroupByName( CATALOG_GROUPNAME );
      var col = new collection( "SYSCAT", "SYSCOLLECTIONSPACES", w.TWO );

      var csName = CHANGEDPREFIX + "_csmetadata"
      options = { PageSize: 4096, LobPageSize: 524288 };
      commCreateCS( db, csName, true, "", options );
      var cond = buildCond( options, csName );
      if( !group.checkResult( true, group.checkDoc, col, cond ) )
      {
          throw new Error( "after createCS metadata is not consistency" );  
      }
      commDropCS( db, csName );
      if( !group.checkResult( true, group.checkDoc, col, cond ) )
      {
         throw new Error( "after dropCS metadata is not consistency" );
      }
   }
   catch( e )
   {
      if( e.constructor === Error )
      {
         println( e.stack );
      }
      throw e;
   }
   finally
   {
      if( undefined !== db )
      {
         db.close();
      }
   }
}

main();
