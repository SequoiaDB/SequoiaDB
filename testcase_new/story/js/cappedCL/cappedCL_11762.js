/************************************
*@Description: create cappedCS and drop cappedCS
*@author:      luweikang
*@createdate:  2017.7.4
*@testlinkCase:seqDB-11762
**************************************/

main( test );
function test ()
{
   var csName = COMMCAPPEDCSNAME + "_11762";

   //clean environment before test
   commDropCS( db, csName, true, "drop CS in the beginning" );

   //begin create cappedCS
   var options = { Capped: true }
   commCreateCS( db, csName, false, "beginning to create cappedCS", options );

   //check create result
   if( true === commIsStandalone( db ) )
   {
      standaloneCheckCreateCS( csName );
   }
   else
   {
      var nodeList = getNodeList( CATALOG_GROUPNAME );
      checkCappedCS( csName, nodeList );
   }

   //begin to drop cappedCS
   commDropCS( db, csName, false, "beginning to drop cappedCS" );

   //check drop result
   if( true === commIsStandalone( db ) )
   {
      standaloneCheckDropCS( csName );
   }
   else
   {
      checkDropCS( csName, nodeList );
   }
}

function standaloneCheckCreateCS ( csName )
{
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { 'Name': csName } );
   var type = cursor.next().toObj().Type;
   assert.equal( type, 1 );
   cursor.close();
}

function standaloneCheckDropCS ( csName )
{
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.getCS( csName );
   } );
}

function getNodeList ( groupName )
{
   var nodeList = [];
   var groupInfo = db.getRG( groupName ).getDetail().current().toObj().Group;
   for( var i in groupInfo )
   {
      var nodeInfo = groupInfo[i].HostName + ":" + groupInfo[i].Service[0].Name;
      nodeList.push( nodeInfo );
   }

   return nodeList;
}

function checkCappedCS ( csName, nodeList )
{
   var clSize = 0;
   for( var i in nodeList )
   {
      var catadb = new Sdb( nodeList[i] );
      clSize = catadb.SYSCAT.SYSCOLLECTIONSPACES.count( { 'Name': csName } );
      //judge the current node exist CL or not
      if( clSize == 0 )
      {
         // wait for the slave node sync
         var waitTime = 180 ; // 3min
         while( waitTime > 0 )
         {
            sleep( 5 * 1000 );
            clSize = catadb.SYSCAT.SYSCOLLECTIONSPACES.count( { 'Name': csName } );
            if( clSize != 0 )
            {
               break;
            }
            waitTime -= 5;
         }
         if( clSize == 0 )
         {
            throw new Error( "wait for node " + catadb + " sync timeout!" );
         }
      }
      var cursor = catadb.SYSCAT.SYSCOLLECTIONSPACES.find( { 'Name': csName } );
      var type = cursor.next().toObj().Type;
      assert.equal( type, 1 );
      cursor.close();

      catadb.close();
   }
}

function checkDropCS ( csName, nodeList )
{
   var clSize = 1 ;
   for( var i in nodeList )
   {
      var catadb = new Sdb( nodeList[i] );
      clSize = catadb.SYSCAT.SYSCOLLECTIONSPACES.count( {'Name': csName } );
      //judge the current node exist CL or not
      if( clSize != 0 )
      {
         // wait for the slave node sync
         var waitTime = 180 ; // 3min
         while( waitTime > 0 )
         {
            sleep( 5 * 1000 );
            clSize = catadb.SYSCAT.SYSCOLLECTIONSPACES.count( { 'Name': csName } );
            if( clSize == 0 )
            {
               break;
            }
            waitTime -= 5;
         }
         if( clSize != 0 )
         {
            throw new Error( "wait for node " + catadb + " sync timeout!" );
         }
      }

      catadb.close();
   }
}
