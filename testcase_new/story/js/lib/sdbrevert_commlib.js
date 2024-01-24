import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

function getReplicalogPath ( db, nodeName )
{
   var cursor = db.snapshot( SDB_SNAP_CONFIGS, { NodeName: nodeName }, { logpath: 1 } );
   while( cursor.next() )
   {
      var replicalogPath = cursor.current().toObj().logpath;
   }
   cursor.close();
   return replicalogPath;
}

function getCoordUrl ( sdb )
{
   var coordUrls = [];
   var rgInfo = sdb.getCoordRG().getDetail().current().toObj().Group;
   for( var i = 0; i < rgInfo.length; i++ )
   {
      var hostname = rgInfo[i].HostName;
      var svcname = rgInfo[i].Service[0].Name;
      coordUrls.push( hostname + ":" + svcname );
   }
   return coordUrls;
}

function generateRandomString ( length )
{
   var characters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
   var result = '';
   var charactersLength = characters.length;

   for( var i = 0; i < length; i++ )
   {
      var randomIndex = Math.floor( Math.random() * charactersLength );
      result += characters.charAt( randomIndex );
   }

   return result;
}
