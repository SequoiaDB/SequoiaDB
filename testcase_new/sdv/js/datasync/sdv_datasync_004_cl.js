/****************************************************************
*@Description: createCL, Collection is exist in all node of group
               dropCL,Collection is not exist in all node of group
*@author:     wangwenjing
*****************************************************************/
function main ()
{
   try
   {
      var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      if( commIsStandalone( db ) )
      {
         return;
      }

      var clName = COMMCLNAME + "_testcl";
      var mgr = new groupMgr( db );
      mgr.init();

      var nodeNum = 2;
      var group = selectGroup( mgr, nodeNum );

      var cl = new collection( COMMCSNAME, clName, w.ALL );
      cl.create( db, group.name );
      cl.bulkInsert( 10 );
      var needSleep = isNeedSleep( cl.replSize );
      if( !group.checkResult( needSleep, group.checkDoc, cl, {} ) )
      {
         throw new Error( "collection is not exist in all group node" );
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
