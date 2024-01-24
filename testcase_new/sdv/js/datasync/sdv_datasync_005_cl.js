/************************************
*@Description: createIndex, index is exist in all node of group
               dropIndex,index is not exist in all node of group
*@author:     wangwenjing
**************************************/
function main ()
{
   try
   {

      var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      if( commIsStandalone( db ) )
      {
         return;
      }

      var clName = COMMCLNAME + "_testidx";
      var mgr = new groupMgr( db );
      mgr.init();

      var nodeNum = 2;
      var group = selectGroupByNodeNum( mgr, nodeNum );

      var cl = new collection( COMMCSNAME, clName, w.ALL );
      cl.create( db, group.name );
      cl.bulkInsert( 10 );
      cl.createIndex( 'idxa', { a: 1 } );
      var needSleep = isNeedSleep( cl.replSize );
      if( !group.checkResult( needSleep, group.checkExplain, cl, { a: 1 } ) )
      {
         throw new Error( "collection's explain is not consistency in all group node" );
      }
      cl.dropIndex( 'idxa' );
      if( !group.checkResult( needSleep, group.checkExplain, cl, { a: 1 } ) )
      {
         throw new Error( "collection's explain is not consistency in all group node" );
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
      if( "undefined" !== db )
      {
         db.close();
      }
   }
}
main();
