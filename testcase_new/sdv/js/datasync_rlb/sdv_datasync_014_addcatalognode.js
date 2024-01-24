/**********************************************************
*@Description: the catalog group add node 
               
*@author:     wangwenjing
***********************************************************/

function createNodeOfCataGroup ( cataGroup )
{
   var groupsArray = commGetGroups( db );
   var hostName = groupsArray[0][1].HostName;
   var port = allocPort();
   var dbPath = RSRVNODEDIR + "/cata/" + port;

   println( hostName + ":" + port + " " + dbPath );
   var node = new replicaNode( hostName, port, cataGroup );
   node.setDbPath( dbPath );

   node.create();

   return node;
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
      var node = createNodeOfCataGroup( group );

      if( !group.checkResult( true, group.checkLSN ) )
      {
         throw new Error( "the LSN is not consistent" );
      }
      if( !group.checkResult( true, group.checkCS ) )
      {
         throw new Error( "system collection space is not consistent" );
      }
      if( !group.checkResult( true, group.checkCL ) )
      {
         throw new Error( "system collection is not consistent" );
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
      if( undefined !== node )
      {
         node.drop();
      }

      if( undefined !== db )
      {
         db.close();
      }
   }
}

main();
