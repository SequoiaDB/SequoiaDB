/******************************************************************************
 * Reliability helpers for dr_location command-line tests.
 ******************************************************************************/

import( "../drLocation/commlib.js" );

function drWaitNodeReportedError ( nodeName, timeoutSec )
{
   for( var second = 0; second < timeoutSec; ++second )
   {
      var found = false;
      var option = new SdbSnapshotOption().options( { ShowError: "only" } );
      var cursor = db.snapshot( SDB_SNAP_DATABASE, option );
      try
      {
         while( cursor.next() )
         {
            var current = cursor.current().toObj();
            if( current.NodeName === nodeName )
            {
               found = true;
               break;
            }

            var errNodes = current.ErrNodes || [];
            for( var i = 0; i < errNodes.length; ++i )
            {
               if( errNodes[i].NodeName === nodeName )
               {
                  found = true;
                  break;
               }
            }
            if( found ) break;
         }
      }
      finally
      {
         cursor.close();
      }

      if( found ) return;
      sleep( 1000 );
   }

   throw new Error( "Node was not reported abnormal within " + timeoutSec +
      " seconds: " + nodeName );
}

function drWaitGroupMaster ( groupName, timeoutSec )
{
   var lastError = null;
   for( var second = 0; second < timeoutSec; ++second )
   {
      try
      {
         return db.getRG( groupName ).getMaster();
      }
      catch( e )
      {
         lastError = e;
         sleep( 1000 );
      }
   }

   throw new Error( "Replica group did not elect a primary within " +
      timeoutSec + " seconds: " + groupName + ", last error: " + lastError );
}
