import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

function createDataGroups ( db, hostNames, groupNum )
{
   var dataGroupNames = [];
   var rgName = "";
   var hostNameIndex = 0;
   try
   {
      for( var i = 0; i < groupNum; i++ )
      {
         var port = parseInt( RSRVPORTBEGIN ) + ( i * 10 );
         rgName = "group15669_" + i;
         var dataRG = db.createRG( rgName );
         dataGroupNames.push( rgName );
         if( hostNameIndex == hostNames.length )
         {
            hostNameIndex = 0;
         }
         dataRG.createNode( hostNames[hostNameIndex], port, RSRVNODEDIR + "data/" + port );
         dataRG.start();
         hostNameIndex++;
      }
      return dataGroupNames;

   } catch( e )
   {
      removeDataRG( db, dataGroupNames );
      throw new Error( "create data RG " + rgName + " failed , errorCode = " + e );
   }
}

function removeDataRG ( db, dataRGNames )
{
   for( var i = 0; i < dataRGNames.length; i++ )
   {
      db.removeRG( dataRGNames[i] );
   }
}