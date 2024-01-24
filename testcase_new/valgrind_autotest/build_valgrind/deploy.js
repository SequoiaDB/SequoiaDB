/****************************************************************
@decription:   Deploy SequoiaDB

@input:        hosts:      Array , one host    ---- 1g3d
two hosts   ---- 2g3d
three hosts ---- 3g3d
eg: bin/sdb -f deploy.js -e 'var hosts=["192.168.31.1", "192.168.31.2", "192.168.31.3"]'

@author:       Yin Zhen 2019-09-12
****************************************************************/

// check parameter
if( typeof ( hosts ) === "undefined" )
{
   var hosts = ["localhost"];
}
else if( hosts.constructor !== Array )
{
   throw "Invalid param[hosts], should be Array";
}

// set global variable
var HOSTS = hosts;
var TMP_COORD_SVC = 18800;
var LOCAL_CM_PORT = 11790;

// run!
main();

function main ()
{
   if( HOSTS.length !== 0 )
   {
      deploySequoiadb();
   }
   else
   {
      println( "Do not deploy sdb, because of hosts: " + HOSTS );
   }
}

// return obj:
// {
//   "NAME": "sdbcm" , 
//   "SDBADMIN_USER": "sdbadmin" , 
//   "INSTALL_DIR": "/opt/source/sequoiadb/" , 
//   "MD5": "818cea64849dff4c1b572a6d6af5d757"
// }
function getSequoiadbInstallInfo ( hostName )
{
   try
   {
      var oma = new Oma( hostName, LOCAL_CM_PORT );
   }
   catch( e )
   {
      println( "Unexpected error[" + e + "] when connecting cm[" + hostName +
         ":" + LOCAL_CM_PORT + "]!" );
      throw e;
   }

   try
   {
      var cmPort = oma.getAOmaSvcName( hostName );
      var omaRemote = new Oma( hostName, cmPort );
   }
   catch( e )
   {
      println( "Unexpected error[" + e + "] when connecting cm[" + hostName +
         ":" + cmPort + "]!" );
      throw e;
   }

   var installInfo = {};
   try
   {
      installInfo = omaRemote.getOmaInstallInfo().toObj();
   }
   catch( e )
   {
      if( e == -4 )
      {
         println( "ERROR: This machine has not installed SequoiaDB!" );
         throw "ERROR";
      }
      else
      {
         throw e;
      }
   }

   return installInfo;
}

function createTmpCoord ( hostName )
{
   var oma = new Oma( hostName, LOCAL_CM_PORT );

   var svcName = TMP_COORD_SVC;
   var installedPath = getSequoiadbInstallInfo( hostName ).INSTALL_DIR;

   try
   {
      oma.createCoord( svcName, installedPath + "/database/coord/" + svcName );
   }
   catch( e )
   {
      if( e != -145 )// -145: already exists , ignore error
      {
         println( "Unexpected error[" + e + "] when creating temp coord: " +
            "localhost:" + svcName + "!" );
         throw e;
      }
   }

   oma.startNode( svcName );
}

function checkCataPrimary ( db )
{
   var hasPrimary = false;

   for( var i = 0; i < 10 * 600; i++ )//wait for cata group to select primary node
   {
      try
      {
         sleep( 100 );
         var cataRG = db.getRG( "SYSCatalogGroup" );
         hasPrimary = true;
         break;
      }
      catch( e )
      {
         if( e !== -71 )
         {
            println( "Unexpected error[" + e + "] when " +
               "db.getRG( \"SYSCatalogGroup\" )!" );
            throw e;
         }
      }
   }

   if( hasPrimary === false )
   {
      println( "Fail to select primary node in group[SYSCatalogGroup] " +
         "after 10 minute" );
      return false;
   }

   return true;
}

function checkeDataPrimary ( db, groupName )
{
   var hasPrimary = false;

   for( var i = 0; i < 10 * 600; i++ )//wait for data group to select primary node
   {
      try
      {
         sleep( 100 );
         db.getRG( groupName ).getMaster();
         hasPrimary = true;
         break;
      }
      catch( e )
      {
         if( e !== -71 )
         {
            println( "Unexpected error[" + e + "] when getting group[" +
               groupName + "]!" );
            throw e;
         }
      }
   }

   if( hasPrimary === false )
   {
      println( "Fail to select primary node in group[" + groupName +
         "] after 10 minute" );
      return false;
   }

   return true;
}

function createCatalog ( db, hostName, svcName, dbPath )
{
   try
   {
      db.createCataRG( hostName, svcName, dbPath );
   }
   catch( e )
   {
      if( e != -200 && e != -145 )
      {
         println( "Unexpected error[" + e + "] when creating catalog " +
            "node: " + hostName + ":" + svcName + "!" );
         throw e;
      }
   }
   try
   {
      var rg = db.getCatalogRG();
      var node = rg.createNode( hostName, svcName, dbPath );
   }
   catch( e )
   {
      if( e != -145 )// -145: already exists , ignore error
      {
         println( "Unexpected error[" + e + "] when creating catalog " +
            "node: " + hostName + ":" + svcName + "!" );
         throw e;
      }
   }
   try
   {
      rg.getNode( hostName, svcName ).start();
   }
   catch( e )
   {
      println( "Unexpected error[" + e + "] when starting catalog node: "
         + hostName + ":" + svcName + "!" );
      throw e;
   }

   println( "Create catalog: " + hostName + ":" + svcName + " [" + dbPath + "]" );
}

function createCoord ( db, hostName, svcName, dbPath )
{

   try
   {
      db.createCoordRG();
   }
   catch( e )
   {
      if( e != -153 )// -153: group already exist , ignore error
      {
         println( "Unexpected error[" + e + "] when creating coord group!" );
         throw e;
      }
   }

   try
   {
      var rg = db.getCoordRG();
      rg.createNode( hostName, svcName, dbPath );
   }
   catch( e )
   {
      if( e != -145 )// -145: node already exist , ignore error
      {
         println( "Unexpected error[" + e + "] when creating coord node!" );
         throw e;
      }
   }

   println( "Create coord:   " + hostName + ":" + svcName + " [" + dbPath + "]" );

   try
   {
      rg.start();
   }
   catch( e )
   {
      println( "Unexpected error[" + e + "] when starting coord group!" );
      throw e;
   }
}

function createData ( db, groupName, hostName, svcName, dbPath )
{
   try
   {
      var rg = db.getRG( groupName );
   }
   catch( e )
   {
      if( e == -154 )
      {
         var rg = db.createRG( groupName );
      }
      else
      {
         println( "Unexpected error[" + e + "] when get data group[" +
            groupName + "]!" );
         throw e;
      }
   }

   try
   {
      rg.createNode( hostName, svcName, dbPath );
   }
   catch( e )
   {
      if( e != -145 )
      {
         println( "Unexpected error[" + e + "] when creating data node[" +
            hostName + ":" + svcName + "]!" );
         throw e;
      }
   }

   println( "Create data:    " + hostName + ":" + svcName + " [" + dbPath + "]" );

   try
   {
      rg.start();
   }
   catch( e )
   {
      println( "Unexpected error[" + e + "] when starting data group[" +
         groupName + "]!" );
      throw e;
   }
}

function removeTmpCoord ( hostName )
{
   var oma = new Oma( hostName, LOCAL_CM_PORT );
   var svcName = TMP_COORD_SVC;

   try
   {
      oma.removeCoord( svcName );
   }
   catch( e )
   {
      println( "Unexpected error[" + e + "] when removing temp coord[localhost:"
         + svcName + "]!" );
      throw e;
   }
}

function getNodeAddress ( type )
{
   var nodeAddrs = [];
   switch( type )
   {
      case "coord":
         for( var i in HOSTS )
         {
            var addr = HOSTS[i] + ":11810";
            nodeAddrs.push( addr );
         }
         break;
      case "catalog":
         if( HOSTS.length == 1 )
         {
            var cataNum = 1;
            var addr = HOSTS[0] + ":11820";
            nodeAddrs.push( addr );
         }
         if( HOSTS.length > 1 )
         {
            var cataNum = 3;
            var cataSvcName = 11820;
            for( var i = 0; i < cataNum; i++ )
            {
               var t = i % HOSTS.length;
               if( i != 0 && t == 0 )
               {
                  cataSvcName = cataSvcName + 10;
               }
               var addr = HOSTS[t] + ":" + cataSvcName;
               nodeAddrs.push( addr );
            }
         }
         break;
      case "data":
         var dataNum = HOSTS.length * 3;
         var dataSvcName = 11830;
         var start = 0;
         var end = dataNum;
         if( HOSTS.length == 2 )
         {
            start = 1;
            end = dataNum + 1;
         }
         for( var i = start; i < end; i++ )
         {
            var t = i % HOSTS.length;
            if( i != 0 && t == 0 )
            {
               dataSvcName = dataSvcName + 10;
            }
            var addr = HOSTS[t] + ":" + dataSvcName;
            nodeAddrs.push( addr );
         }
         break;
      default:
         throw "Cannot match any node type";
   }
   return nodeAddrs;
}

function deploySequoiadb ()
{
   println( "\n************ Deploy SequoiaDB ************************" );

   // check it has installation or not
   for( var i in HOSTS )
   {
      var hostName = HOSTS[i];
      var installInfo = getSequoiadbInstallInfo( hostName );
      if( installInfo == undefined )
      {
         throw "ERROR";
      }
      println( JSON.stringify( installInfo ) );
   }

   // create sequoiadb cluster
   var hostName = HOSTS[0];
   var installPath = installInfo.INSTALL_DIR;

   // TMP_COORD_SVC
   var removeTmpCoordFlag = true;
   try
   {
      var db = new Sdb( hostName, 11810 );
      removeTmpCoordFlag = false;
   }
   catch( e )
   {
      createTmpCoord( hostName );
      var db = new Sdb( hostName, TMP_COORD_SVC );
   }
   var cataAddrs = getNodeAddress( "catalog" );
   var dataAddrs = getNodeAddress( "data" );
   var coordAddrs = getNodeAddress( "coord" );

   for( var i in cataAddrs )
   {
      var addr = cataAddrs[i];
      var host = addr.split( ":" )[0];
      var svcName = addr.split( ":" )[1];
      createCatalog( db, host, svcName, installPath + "/database/catalog/" + svcName );
   }

   var rc = checkCataPrimary( db );
   if( !rc )
   {
      println( "Failed to wait catalog group change primary!" );
      throw "ERROR";
   }

   for( var i in dataAddrs )
   {
      var addr = dataAddrs[i];
      var host = addr.split( ":" )[0];
      var svcName = addr.split( ":" )[1];
      var groupName = "group" + ( parseInt( i / 3 ) + 1 );
      createData( db, groupName, host, svcName, installPath + "/database/data/" + svcName );
   }

   for( var i in HOSTS )
   {
      var groupName = "group" + ( parseInt( i ) + 1 );
      var rc = checkeDataPrimary( db, groupName );
      if( !rc )
      {
         println( "Failed to wait data " + groupName + " change primary!" );
         throw "ERROR";
      }
   }

   for( var i in coordAddrs )
   {
      var addr = coordAddrs[i];
      var host = addr.split( ":" )[0];
      var svcName = addr.split( ":" )[1];
      createCoord( db, host, svcName, installPath + "/database/coord/" + svcName );
   }

   if( removeTmpCoordFlag )
   {
      removeTmpCoord( hostName );
   }
   println( "************ Finish Deploy SequoiaDB *****************" );
}

