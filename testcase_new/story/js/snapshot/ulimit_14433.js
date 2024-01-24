/*******************************************************************
* @Description : test health snapshot field "Ulimit"
*                seqDB-14433:Ulimit字段测试
* @author      : linsuqiang
* 
*******************************************************************/
main( test );

function test ()
{
   var remote = new Remote( COORDHOSTNAME, 11790 );
   var cmd = remote.getCmd();
   var nodeInfo = getLocalNodeInfo( cmd );
   var expUlimit = getUlimitByCmd( cmd, nodeInfo.PID );
   var actUlimit = getUlimitBySnap( db, nodeInfo.GID, nodeInfo.NID );
   isEquals( expUlimit, actUlimit );
}

function getUlimitByCmd ( cmd, PID )
{
   var ulimitJson = {};
   ulimitJson.CoreFileSize = getUlimitItem( cmd, PID, "Max core file size" );
   ulimitJson.VirtualMemory = getUlimitItem( cmd, PID, "Max address space" );
   ulimitJson.OpenFiles = getUlimitItem( cmd, PID, "Max open files" );
   ulimitJson.NumProc = getUlimitItem( cmd, PID, "Max processes" );
   ulimitJson.FileSize = getUlimitItem( cmd, PID, "Max file size" );
   return ulimitJson;
}

function getUlimitItem ( cmd, PID, itemName )
{
   var result = cmd.run( "cat /proc/" + PID + "/limits | " +
      "grep '" + itemName + "' | " +
      "awk -F '  +' '{print $2}'" );
   result = result.slice( 0, result.length - 1 );
   if( result === "unlimited" )
   {
      result = -1;
   }
   result = parseInt( result );
   return result;
}

function getUlimitBySnap ( db, GID, NID )
{
   var cond = {};
   if( GID != '-' ) // '-' means standalone
   {
      GID = parseInt( GID );
      NID = parseInt( NID );
      cond.NodeID = { $et: [GID, NID] };
   }

   var cur = db.snapshot( SDB_SNAP_HEALTH, cond );
   var result = cur.next().toObj().Ulimit;
   cur.close();
   return result;
}

function getLocalNodeInfo ( cmd )
{
   var installPath = commGetRemoteInstallPath( COORDHOSTNAME, CMSVCNAME );
   var infoStr = cmd.run( installPath + "/bin/sdblist -l | grep sequoiadb | head -n 1" );
   if( infoStr == "" )
   {
      throw new Error( "no any node of localhost" );
   }
   var infoArr = infoStr.split( /\s+/ );
   var infoJson = {};
   infoJson.Name = infoArr[0];
   infoJson.SvcName = infoArr[1];
   infoJson.Role = infoArr[2];
   infoJson.PID = infoArr[3];
   infoJson.GID = infoArr[4];
   infoJson.NID = infoArr[5];
   infoJson.PRY = infoArr[6];
   infoJson.GroupName = infoArr[7];
   infoJson.StartTime = infoArr[8];
   infoJson.DBPath = infoArr[9];
   return infoJson;
}

function isEquals ( a, b )
{
   if( a instanceof Object && b instanceof Object )
   {
      var aProps = Object.getOwnPropertyNames( a );
      var bProps = Object.getOwnPropertyNames( b );

      if( aProps.length != bProps.length )
      {
         return "aProps.length is " + aProps.length + ", bProps.length " + bProps.length;
      }

      for( var i = 0; i < aProps.length; ++i )
      {
         var propName = aProps[i];
         if( a[propName] !== b[propName] )
         {
            throw new Error( "a[propName] is " + a[propName] + ", b[propName] is " + b[propName] );
         }
      }
   }
   else
   {
      throw new Error( "typeof a is " + typeof a + ", typeof b is " + typeof b );
   }
}
