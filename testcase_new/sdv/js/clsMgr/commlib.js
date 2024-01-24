/* *****************************************************************************
@Description: insert data and check data
@author:      Zhao Xiaoni
***************************************************************************** */
import( "../lib/main.js" );

function insertData( cl, num )
{
   var records = [];
   for( var i = 0; i < num; ++i )
   {
      var record = { "_id": i, "b": 2 * i, "c": false, "d": { "da": [ i, i + 1, "oo" ], "db": { "dba": "test" } } };
      records.push( record );
   }
   cl.insert( records );

   return records;
}

function getHostName()
{
   var remote = new Remote( COORDHOSTNAME, CMSVCNAME );
   var cmd = remote.getCmd();
   var hostName = cmd.run( "hostname" ).split( /(\n)/ )[0];
   return hostName;
}

function generatePort( port )
{
   var cmd = new Cmd();
   port = parseInt( port ) + 10;
   try
   {
      cmd.run( "lsof -i:" + port );
      port = generatePort( port );
   }
   catch( e )
   {
      if( e !== 1 )
      {
         throw new Error( e );
      }
   }
   return port.toString();
}

function createDb( hostName, port, isStandalone )
{
   if( undefined === isStandalone ) 
   {
      isStandalone = false;
   }

   var oma = new Oma( hostName, CMSVCNAME );
   if( isStandalone )
   {
      oma.createData( port, RSRVNODEDIR + "standalone/" + port );
      oma.startNode( port );
   }
   else
   {  
      oma.createCoord( port, RSRVNODEDIR + "coord/" + port );
      oma.startNode( port );
   }

   var db = new Sdb( hostName, port );
   return db;
}

function createCata ( db, hostName )
{
   var cataPorts = [ generatePort( CATASVCNAME ) ];
   db.createCataRG( hostName, cataPorts[0], RSRVNODEDIR + "cata/" + cataPorts[0] );
   var cataRG = db.getRG("SYSCatalogGroup");
   var cataPort = createNode( cataRG, hostName, CATASVCNAME, RSRVNODEDIR + "cata/" );
   cataPorts.push( cataPort );
   return cataPorts;
}

function createData ( db, hostName, groupName )
{
   var dataRG = db.createRG( groupName );
   var dataPort1 = createNode( dataRG, hostName, RSRVPORTBEGIN, RSRVNODEDIR + "data/" );
   var dataPort2 = createNode( dataRG, hostName, RSRVPORTBEGIN, RSRVNODEDIR + "data/" );
   dataRG.start();
   var dataPorts = [ dataPort1, dataPort2 ];
   return dataPorts;
}

function createCoord ( db, hostName )
{
   var coordRG = db.createCoordRG();
   var coordPort = createNode( coordRG, hostName, COORDSVCNAME, RSRVNODEDIR + "coord/" );
   coordRG.start();
   return coordPort;
}
  
function createNode( rg, hostName, svcName, path )
{
   var doTimes = 0;
   var timeOut = 20;
   var svcNames = [];
   var svcName = parseInt( svcName );
   var dbPath = path + svcName;
   do
   {
      try
      {
         rg.createNode( hostName, svcName, dbPath, { "diaglevel": 5 } );
         println( "create node: " + hostName + ":" + svcName + " dbpath: " + dbPath );
         svcNames.push( svcName );
         break;
      }
      catch( e )
      {
         //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
         if( commCompareErrorCode( e, -145 ) || commCompareErrorCode( e, -290 ) )
         {
            svcName = svcName + 10;
            dbPath = path + svcName;
            doTimes++;
         }
         else
         {
            throw new Error( "create node failed!  svcName = " + svcName + " dataPath = " + dbPath + " errorCode: " + e );
         }
      }
   }
   while( doTimes < timeOut );
   return svcNames;
} 

function clean( db, hostName, port, groupName, type )
{
   switch( type )
   {
      case "coord_node":
         db.getCoordRG().removeNode( hostName, port );
         break;
      case "cata_node":
         db.getCatalogRG().removeNode( hostName, port );
         break;
      case "data_node":
         db.getRG( groupName ).removeNode( hostName, port );
         break;
      case "tmp_coord":
         var oma = new Oma( hostName, CMSVCNAME );
         oma.removeCoord( port );
         break;
      case "standalone_data":
         var oma = new Oma( hostName, CMSVCNAME );
         oma.removeData( port );
         break;
      case "cata_group":
         db.removeCatalogRG();
         break;
      case "data_group":
         db.removeRG( groupName );
         break;
      case "coord_group":
         db.removeCoordRG();
         break;
   }
}
