/*****************************************************
*@Description: finish deploy, check system collection 
*@author:     wangwenjing
******************************************************/
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

      if( !group.checkResult( true, group.checkCS ) )
      {
         throw new Error( "system collection space is not consistency" );
      }
      if( !group.checkResult( true, group.checkCL ) )
      {
         throw new Error( "system collection is not consistency" );
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
