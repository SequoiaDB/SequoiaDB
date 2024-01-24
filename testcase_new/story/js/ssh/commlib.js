import( "../lib/main.js" )
import( "../lib/basic_operation/commlib.js" );

/******************************************************************************
*@Description : common parameters
*@auhor       : Liang XueWang
******************************************************************************/
var user = REMOTEUSER;
var password = REMOTEPASSWD;
var port = 22;

/******************************************************************************
*@Description : get remote 
*@author      : Zhao xiaoni
******************************************************************************/
function getRemote ( hostName )
{
   var remote = new Remote( hostName, CMSVCNAME );
   return remote;
}

/******************************************************************************
*@Description : get remote cmd
*@author      : Zhao xiaoni
******************************************************************************/
function getRemoteCmd ( hostName )
{
   var remote = getRemote( hostName );
   var cmd = remote.getCmd();
   return cmd;
}

/******************************************************************************
*@Description : get hosts in cluster
*@author      : Liang XueWang
******************************************************************************/
function getHostNames ()
{
   var hostNames = [];
   var cursor = db.listReplicaGroups();
   while( cursor.next() )
   {
      var group = cursor.current().toObj().Group;
      for( var i = 0; i < group.length; i++ )
      {
         var hostName = group[i].HostName;
         if( hostNames.indexOf( hostName ) == -1 )
         {
            hostNames.push( hostName );
         }
      }
   }
   return hostNames;
}

/******************************************************************************
*@Description : get local hostname( COORDHOSTNAME )
*               localhost means cluster local host, host of COORDHOSTNAME
*@author      : Liang XueWang
******************************************************************************/
function getLocalHostName ()
{
   var cmd = getRemoteCmd( COORDHOSTNAME );
   var hostName = cmd.run( "hostname" ).split( "\n" )[0];
   return hostName;
}

/******************************************************************************
*@Description : get remote hostname
*@author      : Liang XueWang
******************************************************************************/
function getRemoteHostName ()
{
   var hostNames = getHostNames();
   var localHostName = getLocalHostName();
   var remoteHostName = localHostName;
   for( var i = 0; i < hostNames.length; i++ )
   {
      if( hostNames[i] !== localHostName )
      {
         remoteHostName = hostNames[i];
         break;
      }
   }
   return remoteHostName;
}

/******************************************************************************
*@Description : check cm user
*@author      : Liang XueWang
******************************************************************************/
function checkCmUser ( hostName, user )
{
   var remote = new Remote( hostName, CMSVCNAME );
   var actual = remote.getSystem().getCurrentUser().toObj().user;
   remote.close();
   if( user !== actual )
   {
      println( "Remote user is not " + hostName );
      return false;
   }
   return true;
}

/******************************************************************************
*@Description : check system has user
*@author      : Liang XueWang
******************************************************************************/
function isUserExist ( hostName, user )
{
   var remote = new Remote( hostName, CMSVCNAME );
   var cmd = remote.getCmd();
   try
   {
      cmd.run( "cat /etc/passwd | grep " + user );
   }
   catch( e )
   {
      if( e.message == 1 )
      {
         println( "There is no user: " + user );
         return false;
      }
      else
      {
         throw e;
      }
   }
   return true;
}

/******************************************************************************
*@Description : clean up local file
*@author      : Zhao xiaoni
******************************************************************************/
function cleanLocalFile ( fileName )
{
   var cmd = new Cmd();
   cmd.run( "rm -rf " + fileName );
}

/******************************************************************************
*@Description : clean up remote file
*@author      : Zhao xiaoni
******************************************************************************/
function cleanRemoteFile ( remoteHostName, remoteSvcName, fileName )
{
   cmd = getRemoteCmd( remoteHostName, remoteSvcName );
   cmd.run( "rm -rf " + fileName );
}

/******************************************************************************
*@Description : get ip address of hostname
*@author      : Liang XueWang
******************************************************************************/
function getIPAddr ( hostName )
{

   if( hostName == "localhost" || hostName == "127.0.0.1" )
   {
      ip = "127.0.0.1";
   }
   else
   {
      var cmd = getRemoteCmd( hostName );
      var command = "cat /etc/hosts | grep " + hostName + " | grep -v 127 | grep -v '#' | awk '{print $1}'";
      ip = cmd.run( command ).split( "\n" )[0];
   }
   return ip;
}

/******************************************************************************
*@Description : check remote file mode and content
*@author      : Liang XueWang
******************************************************************************/
function checkRemoteFile ( hostName, fileName, mode, content )
{
   var remote = getRemote( hostName, CMSVCNAME );
   var file = remote.getFile( fileName );

   var actual = file.stat( fileName ).toObj().mode;
   if( mode !== actual )
   {
      throw new Error( "mode: " + mode + ", actual: " + actual );
   }

   actual = file.read().split( "\n" )[0];
   if( content !== actual )
   {
      throw new Error( "content: " + content + ", actual: " + actual );
   }

   file.close();
}
/******************************************************************************
*@Description : check local file mode and content
*@author      : Liang XueWang
******************************************************************************/
function checkLocalFile ( fileName, mode, content )
{
   var file = new File( fileName );
   var actual = file.stat( fileName ).toObj().mode;

   if( actual !== mode )
   {
      throw new Error( "mode: " + mode + ", actual: " + actual );
   }

   actual = file.read().split( "\n" )[0];
   if( actual !== content )
   {
      throw new Error( "content: " + content + ", actual: " + actual );
   }
   file.close();
}
