/*******************************************************************
* @Description : stop and start multi groups
*                seqDB-14663:停止、启动多个数据组
* @author      : Liang XueWang
*                2018-03-12
*******************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   //var groups = getDataGroups( db );
   var groups = commGetDataGroupNames( db );
   if( groups.length <= 1 )
   {
      return;
   }
   var group1 = groups[0];
   var group2 = groups[1];

   // stop group1 group2
   db.stopRG( group1, group2 );

   // check stop
   checkGroupStatus( db, group1, "stop" );
   checkGroupStatus( db, group2, "stop" );

   // start group1 group2
   db.startRG( group1, group2 );

   // check start
   checkGroupStatus( db, group1, "start" );
   checkGroupStatus( db, group2, "start" );

   // wait primary choosed
   waitPrimary( db, group1 );
   waitPrimary( db, group2 );
}

function checkGroupStatus ( db, groupname, status )
{
   var nodes = commGetGroupNodes( db, groupname );
   for( var i = 0; i < nodes.length; i++ )
   {
      try
      {
         var dataDb = new Sdb( nodes[i].HostName + ":" + nodes[i].svcname );
         if( status === "stop" ) throw new Error( "should error" );
      }
      catch( e )
      {
         if( status === "stop" && ( e.message == SDB_NETWORK || e.message == SDB_NET_CANNOT_CONNECT ) )
         { } // when group stopped, connect throw -15 or -79, do nothing
         else
         {
            var expectErr = ( status === "stop" ) ? -15 : 0;
            throw new Error( "check node: " + nodes[i].HostName + ":" + nodes[i].svcname + " status failed, e:" + e );
         }
      }
   }

}

function waitPrimary ( db, groupname )
{
   //定义最长无主时间，超过600s就抛错
   var timeOut = 60 * 10;
   var i = 0;
   var isPrimary = false;
   do
   {
      try
      {
         db.getRG( groupname ).getMaster();
         isPrimary = true;
         i++;
      }
      catch( e )
      {
         if( i < timeOut )
         {
            println( "waitPrimary throw " + e );
            sleep( 1000 );
            continue;
         }
         else
         {
            throw e;
         }

      }
   }
   while( !isPrimary );
}
