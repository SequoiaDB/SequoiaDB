/**********************************************************
decription: the data group add node                
*@author:     wangwenjing
***********************************************************/
function createNodeOfDataGroup ( dataGroup )
{
   var hostName = getHostNameOfLocal();
   var port = allocPort();
   var dbPath = buildDeployPath( port, "data" );

   var node = new replicaNode( hostName, port, dataGroup );
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

      var nodeNum = 1;
      var group = selectGroupByNodeNum( mgr, nodeNum )


      var clName = COMMCLNAME + "_testdatasync";
      var cl = new collection( COMMCSNAME, clName, w.ONE );
      cl.drop( db );
      cl.create( db, group.name );
      var number = 100000;
      cl.bulkInsert( number );

      var node = createNodeOfDataGroup( group );
      if( !group.checkResult( true, group.checkLSN ) )
      {
         throw new Error( "the LSN is not consistent" );
      }
      if( !group.checkConsistency( cl ) )
      {
         throw new Error( "the data is not consistent" ); 
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

//main();
