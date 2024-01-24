/**********************************************************
*@Description: createCL, check meta data about of Collection
               dropCL,check meta data about of Collection
*@author:     wangwenjing
***********************************************************/

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
      var col = new collection( "SYSCAT", "SYSCOLLECTIONS", w.TWO );

      var clName = COMMCLNAME + "_clmetadata";
      options = { ShardingType: 'hash', ShardingKey: { a: 1 }, EnsureShardingIndex: false };
      commCreateCL( db, COMMCSNAME, clName, options, true );
      var cond = buildCond( options, clName );
      if( !group.checkResult( true, group.checkDoc, col, cond ) )
      {
         throw new Error( "after createCS metadata is not consistency" );
      }
      commDropCL( db, COMMCSNAME, clName );
      if( !group.checkResult( true, group.checkDoc, col, cond ) )
      {
         throw new Error( "after createCS metadata is not consistency" );
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