/************************************
*@Description: create cappedCL and drop cappedCL
*@author:      luweikang
*@createdate:  2017.7.6
*@testlinkCase:seqDB-11763
**************************************/

main( test );
function test ()
{
   //create cappedCL
   var clName = COMMCAPPEDCLNAME + "_11763";
   var optionObj = { Capped: true, Size: 1024, Max: 10000000, AutoIndexId: false };
   commCreateCL( db, COMMCAPPEDCSNAME, clName, optionObj, false, true );

   //check cappedCL
   if( true === commIsStandalone( db ) )
   {
      standaloneCheckCreateCL( COMMCAPPEDCSNAME, clName );
   }
   else
   {
      var nodeList = getNodeList( CATALOG_GROUPNAME );
      checkCappedCL( COMMCAPPEDCSNAME, clName, nodeList );
   }

   //drop cappedCL
   commDropCL( db, COMMCAPPEDCSNAME, clName, false, false, "drop cappedCL" );

   //check drop cappedCL result
   if( true === commIsStandalone( db ) )
   {
      standaloneCheckDropCL( COMMCAPPEDCSNAME, clName );
   }
   else
   {
      checkDropCL( COMMCAPPEDCSNAME, nodeList );
   }

   commDropCL( db, COMMCAPPEDCSNAME, clName, true, true, "drop CL in the end" );
}

function standaloneCheckCreateCL ( COMMCAPPEDCSNAME, clName )
{
   var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { 'Name': COMMCAPPEDCSNAME + "." + clName } );
   var attribute = cursor.next().toObj().Details[0].Attribute;
   assert.equal( attribute, "NoIDIndex | Capped" );
   cursor.close();
}

function standaloneCheckDropCL ( COMMCAPPEDCSNAME, clName )
{
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      db.getCS( COMMCAPPEDCSNAME ).getCL( clName );
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

function checkCappedCL ( COMMCAPPEDCSNAME, clName, nodeList )
{
   var repeatTime = 10;
   var clSize = 0;
   for( var i = 0; i < repeatTime; i++ )
   {
      var j = i % nodeList.length;
      var catadb = new Sdb( nodeList[j] );
      clSize = catadb.SYSCAT.SYSCOLLECTIONS.count( { 'Name': COMMCAPPEDCSNAME + "." + clName } );
      //judge the current node exist CL or not 
      if( clSize == 0 )
      {
         // wait for the slave node sync
         sleep( 2 * 60 * 1000 );//2 mins		
         clSize = catadb.SYSCAT.SYSCOLLECTIONS.count( { 'Name': COMMCAPPEDCSNAME + "." + clName } );
      }

      if( clSize != 0 )
      {
         var cursor = catadb.SYSCAT.SYSCOLLECTIONS.find( { 'Name': COMMCAPPEDCSNAME + "." + clName } );
         var obj = cursor.next().toObj();
         var attributeDesc = obj.AttributeDesc;
         var max = obj.Max;
         var size = obj.Size;
         if( attributeDesc !== "NoIDIndex | Capped" || max == undefined
            || size == undefined )
         {
            throw new Error( "check cappedCL attributeDesc check cappedCL attributeDesc NoIDIndex | Capped" + attributeDesc );
         }
         cursor.close();
      } else
      {
         throw new Error( "check cappedCL failed , cursor is null" );
      }
      catadb.close();
   }

}

function checkDropCL ( COMMCAPPEDCSNAME, clName, nodeList )
{
   for( var i in nodeList )
   {
      var catadb = new Sdb( nodeList[i] );
      var count = catadb.SYSCAT.SYSCOLLECTIONS.count( { 'Name': COMMCAPPEDCSNAME + "." + clName } );
      assert.euqal( count, 0 );
      catadb.close();
   }
}