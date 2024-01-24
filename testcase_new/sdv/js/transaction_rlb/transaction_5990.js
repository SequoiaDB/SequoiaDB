/*******************************************************************************
*@Description: seqDB-5990:事务功能关闭，执行事务操作
*@Author:      2020-4-25 Zhao Xiaoni
********************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var coordUrl = getCoordUrl( db );
   if( coordUrl.length < 2 )
   {
      return;
   }

   var db1 = new Sdb( coordUrl[0] );
   try
   {
      db1.updateConf( { "transactionon": false }, { "NodeName": coordUrl[1] } );
      throw "Excute updateConf should be failed!";
   }
   catch( e )
   {
      if( e !== -322 )
      {
         throw new Error( e );
      }
   }
   restartCoord( db1, coordUrl[1] );

   var db2 = new Sdb( coordUrl[1] );
   try
   {
      db2.transBegin();
      throw "Excute transBegin should be failed!";
   }
   catch( e )
   {
      if( e !== -253 )
      {
         throw new Error( e );
      }
   }

   try
   {
      db1.deleteConf( { "transactionon": 1 }, { "NodeName": coordUrl[1] } );
      throw "Excute deleteConf should be failed!";
   }
   catch( e )
   {
      if( e !== -322 )
      {
         throw new Error( e );
      }
   }

   restartCoord( db1, coordUrl[1] );

   db1.close();
   db2.close();
}

function restartCoord ( db, coordNodeName )
{
   var coordRG = db.getCoordRG();
   var coordNode = coordRG.getNode( coordNodeName );
   coordNode.stop();
   coordNode.start();
}

function getCoordUrl ( db )
{
   var coordUrls = [];
   var rgInfo = db.getCoordRG().getDetail().current().toObj().Group;
   for( var i = 0; i < rgInfo.length; i++ )
   {
      var hostname = rgInfo[i].HostName;
      var svcname = rgInfo[i].Service[0].Name;
      coordUrls.push( hostname + ":" + svcname );
   }
   return coordUrls;
}
