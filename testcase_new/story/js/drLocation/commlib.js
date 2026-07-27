/******************************************************************************
 * Common helpers for dr_location command-line tests.
 ******************************************************************************/

import( "../lib/main.js" );

// Skip CI standalone / single-node-per-group environments.
// These suites need multi-node replica groups and Location DR semantics.
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

var drCmd = new Cmd();
var drInstallDir = commGetInstallPath();
if( drInstallDir.charAt( drInstallDir.length - 1 ) !== "/" )
{
   drInstallDir += "/";
}

var drSourceToolDir = drInstallDir + "tools/dr_location";
var drWorkDir = WORKDIR + "/drLocation/";
var drToolDir = drWorkDir + "tool";
var drOutputDir = drToolDir + "/output";
var DR_PARAM_TIMEOUT = 15;
var DR_OPERATION_TIMEOUT = 120;

drCmd.run( "rm -rf " + drShellQuote( drWorkDir ) );
drCmd.run( "mkdir -p " + drShellQuote( drWorkDir ) );
drCmd.run( "cp -R " + drShellQuote( drSourceToolDir ) + " " +
   drShellQuote( drToolDir ) );
drCmd.run( "find " + drShellQuote( drToolDir ) +
   " -type f -name '*.sh' -exec sed -i 's/\\r$//' {} +" );
drWriteFile( drToolDir + "/config/config.js", drBuildConfig( 100, 1000 ) );

function drShellQuote ( value )
{
   return "'" + String( value ).replace( /'/g, "'\"'\"'" ) + "'";
}

function drRun ( script, args, timeoutSec )
{
   if( args === undefined ) args = [];
   if( timeoutSec === undefined ) timeoutSec = DR_PARAM_TIMEOUT;

   var command = "cd " + drShellQuote( drToolDir ) +
      " && timeout " + timeoutSec + "s bash ./bin/" + script;
   for( var i = 0; i < args.length; ++i )
   {
      command += " " + drShellQuote( args[i] );
   }
   command = "{ " + command + "; } 2>&1; " +
      "rc=$?; printf '\\n__DRLOCATION_RC__:%s\\n' \"$rc\"";

   var rawOutput = drCmd.run( command );
   var match = rawOutput.match( /__DRLOCATION_RC__:(\d+)\s*$/ );
   if( match === null )
   {
      throw new Error( "Unable to parse dr_location return code.\n" +
         "Command: " + command + "\nOutput:\n" + rawOutput );
   }
   var markerIndex = rawOutput.lastIndexOf( "__DRLOCATION_RC__:" );
   return {
      command: command,
      rc: parseInt( match[1], 10 ),
      output: rawOutput.substring( 0, markerIndex ).replace( /\s+$/, "" )
   };
}

function drAssertSuccess ( result )
{
   if( result.rc !== 0 )
      throw new Error( "Command should succeed, rc=" + result.rc +
         "\nOutput:\n" + result.output );
}

function drAssertFailure ( result )
{
   if( result.rc === 0 )
      throw new Error( "Command should fail but returned 0.\nOutput:\n" +
         result.output );
   if( result.rc === 124 )
      throw new Error( "Command should fail quickly but timed out.\nOutput:\n" +
         result.output );
}

function drAssertTimeout ( result )
{
   if( result.rc !== 124 )
      throw new Error( "Command should time out, rc=" + result.rc +
         "\nOutput:\n" + result.output );
}

function drAssertContains ( result, expected )
{
   if( result.output.indexOf( expected ) < 0 )
      throw new Error( "Expected output to contain '" + expected +
         "'.\nActual output:\n" + result.output );
}

function drAssertNotContains ( result, unexpected )
{
   if( result.output.indexOf( unexpected ) >= 0 )
      throw new Error( "Expected output not to contain '" + unexpected +
         "'.\nActual output:\n" + result.output );
}

function drWriteFile ( path, content )
{
   var file = new File( path, 0644, SDB_FILE_READWRITE | SDB_FILE_REPLACE);
   try
   {
      file.write( content );
   }
   finally
   {
      file.close();
   }
   return path;
}

function drWriteTempFile ( fileName, content )
{
   var path = drWorkDir + fileName;
   drWriteFile( path, content );
   return path;
}

function drBuildConfig ( minKeepTime, maxKeepTime, initLocationObject,
   activeLocation )
{
   if( initLocationObject === undefined ) initLocationObject = {};
   if( activeLocation === undefined ) activeLocation = "";

   return "sdbCoord = \"" + COORDHOSTNAME + ":" + COORDSVCNAME + "\";\n" +
      "sdbUser = \"\";\n" +
      "sdbPassword = \"\";\n" +
      "sdbToken = \"\";\n" +
      "sdbCipherFile = \"\";\n" +
      "sdbCm = 11790;\n" +
      "initLocationObject = " + JSON.stringify( initLocationObject ) + ";\n" +
      "activeLocation = " + JSON.stringify( activeLocation ) + ";\n" +
      "reelectLevel = \"\";\n" +
      "minKeepTime = " + minKeepTime + ";\n" +
      "maxKeepTime = " + maxKeepTime + ";\n" +
      "enforce = false;\n";
}

function drCaptureClusterState ()
{
   var groups = [];
   var groupCursor = db.list( SDB_LIST_GROUPS );
   try
   {
      while( groupCursor.next() )
      {
         var groupObj = groupCursor.current().toObj();
         var nodes = [];
         var groupNodes = groupObj.Group || [];
         for( var i = 0; i < groupNodes.length; ++i )
         {
            nodes.push( String( groupNodes[i].HostName ) + ":" +
               String( groupNodes[i].NodeID ) + ":" +
               String( groupNodes[i].Location ) );
         }
         nodes.sort();
         groups.push( String( groupObj.GroupName ) + "|" +
            String( groupObj.ActiveLocation ) + "|" + nodes.join( "," ) );
      }
   }
   finally
   {
      groupCursor.close();
   }
   groups.sort();

   var groupModes = [];
   var modeCursor = db.list( SDB_LIST_GROUPMODES );
   try
   {
      while( modeCursor.next() )
      {
         var modeObj = modeCursor.current().toObj();
         groupModes.push( String( modeObj.GroupName ) + "|" +
            String( modeObj.GroupMode ) + "|" +
            JSON.stringify( modeObj.Properties || [] ) );
      }
   }
   finally
   {
      modeCursor.close();
   }
   groupModes.sort();

   return JSON.stringify( { groups: groups, groupModes: groupModes } );
}

function drAssertClusterStateUnchanged ( expectedState )
{
   var actualState = drCaptureClusterState();
   if( actualState !== expectedState )
      throw new Error( "A rejected dr_location command changed cluster state." +
         "\nBefore: " + expectedState + "\nAfter: " + actualState );
}

function drReadTextFile ( path )
{
   var file;
   try
   {
      file = new File( path );
      return file.read();
   }
   catch( e )
   {
      return "";
   }
   finally
   {
      if( file !== undefined ) file.close();
   }
}

function drGetReplicaGroup ( groupName )
{
   if( groupName === "SYSCatalogGroup" ) return db.getCataRG();
   if( groupName === "SYSCoord" ) return db.getCoordRG();
   return db.getRG( groupName );
}

function drCaptureLocationState ()
{
   var state = { nodes: [], groups: [] };
   var cursor = db.list( SDB_LIST_GROUPS );
   try
   {
      while( cursor.next() )
      {
         var group = cursor.current().toObj();
         var groupNodes = group.Group || [];
         for( var i = 0; i < groupNodes.length; ++i )
         {
            var node = groupNodes[i];
            var services = node.Service || [];
            var serviceName = "";
            for( var j = 0; j < services.length; ++j )
            {
               if( services[j].Type === 0 || services[j].Type === "0" )
               {
                  serviceName = String( services[j].Name );
                  break;
               }
            }
            state.nodes.push( {
               groupName: String( group.GroupName ),
               hostName: String( node.HostName ),
               serviceName: serviceName,
               location: node.Location === undefined ? "" :
                  String( node.Location )
            } );
         }
         state.groups.push( {
            groupName: String( group.GroupName ),
            activeLocation: group.ActiveLocation === undefined ? "" :
               String( group.ActiveLocation )
         } );
      }
   }
   finally
   {
      cursor.close();
   }
   return state;
}

function drBuildHostLocationPlan ( state )
{
   var uncoveredGroups = {};
   var hostCoverage = {};
   for( var i = 0; i < state.nodes.length; ++i )
   {
      var node = state.nodes[i];
      if( node.groupName === "SYSCoord" ) continue;
      uncoveredGroups[node.groupName] = true;
      if( hostCoverage[node.hostName] === undefined )
         hostCoverage[node.hostName] = {};
      hostCoverage[node.hostName][node.groupName] = true;
   }

   var allHosts = Object.keys( hostCoverage ).sort();
   if( allHosts.length === 0 )
      throw new Error( "No catalog or data-group host is available" );

   var primaryHosts = [];
   while( Object.keys( uncoveredGroups ).length > 0 )
   {
      var selectedHost = "";
      var selectedCoverage = -1;
      for( var h = 0; h < allHosts.length; ++h )
      {
         var host = allHosts[h];
         var coverage = 0;
         for( var groupName in hostCoverage[host] )
            if( uncoveredGroups[groupName] ) ++coverage;
         if( coverage > selectedCoverage )
         {
            selectedHost = host;
            selectedCoverage = coverage;
         }
      }
      if( selectedCoverage <= 0 )
         throw new Error( "Unable to cover every group with primary Location" );
      primaryHosts.push( selectedHost );
      for( var coveredGroup in hostCoverage[selectedHost] )
         delete uncoveredGroups[coveredGroup];
   }

   var plan = { "dr_location_primary": primaryHosts };
   var remainingHosts = [];
   for( var a = 0; a < allHosts.length; ++a )
      if( primaryHosts.indexOf( allHosts[a] ) < 0 )
         remainingHosts.push( allHosts[a] );
   if( remainingHosts.length > 0 )
      plan["dr_location_same_city"] = [ remainingHosts[0] ];
   if( remainingHosts.length > 1 )
      plan["dr_location_offsite"] = remainingHosts.slice( 1 );
   return plan;
}

function drAssertLocationPlan ( state, plan, activeLocation )
{
   var expected = {};
   for( var location in plan )
      for( var i = 0; i < plan[location].length; ++i )
         expected[plan[location][i]] = location;

   var seen = {};
   for( var n = 0; n < state.nodes.length; ++n )
   {
      var node = state.nodes[n];
      if( expected[node.hostName] === undefined ) continue;
      if( node.location !== expected[node.hostName] )
         throw new Error( "Host " + node.hostName + " expected Location " +
            expected[node.hostName] + ", actual " + node.location );
      seen[node.hostName] = true;
   }
   for( var host in expected )
      if( !seen[host] ) throw new Error( "Expected host missing: " + host );

   for( var g = 0; g < state.groups.length; ++g )
   {
      var group = state.groups[g];
      if( group.groupName === "SYSCoord" ) continue;
      if( group.activeLocation !== activeLocation )
         throw new Error( "Group " + group.groupName +
            " expected ActiveLocation " + activeLocation + ", actual " +
            group.activeLocation );
   }
}

function drRestoreLocationState ( state )
{
   for( var i = 0; i < state.nodes.length; ++i )
   {
      var node = state.nodes[i];
      drGetReplicaGroup( node.groupName ).getNode(
         node.hostName, node.serviceName ).setLocation( node.location );
   }
   for( var j = 0; j < state.groups.length; ++j )
   {
      var group = state.groups[j];
      if( group.groupName === "SYSCoord" ) continue;
      drGetReplicaGroup( group.groupName ).setActiveLocation(
         group.activeLocation );
   }
}

function drAssertLocationStateEquals ( expected, actual )
{
   function sortNodes ( left, right )
   {
      var leftKey = left.groupName + "|" + left.hostName + "|" +
         left.serviceName;
      var rightKey = right.groupName + "|" + right.hostName + "|" +
         right.serviceName;
      return leftKey < rightKey ? -1 : leftKey > rightKey ? 1 : 0;
   }
   function sortGroups ( left, right )
   {
      return left.groupName < right.groupName ? -1 :
         left.groupName > right.groupName ? 1 : 0;
   }
   expected.nodes.sort( sortNodes );
   actual.nodes.sort( sortNodes );
   expected.groups.sort( sortGroups );
   actual.groups.sort( sortGroups );
   if( JSON.stringify( expected ) !== JSON.stringify( actual ) )
      throw new Error( "Location state was not restored.\nExpected: " +
         JSON.stringify( expected ) + "\nActual: " + JSON.stringify( actual ) );
}
function drListGroupModes ( groupName )
{
   var modes = [];
   var matcher = groupName === undefined ? {} : { GroupName: groupName };
   var cursor = db.list( SDB_LIST_GROUPMODES, matcher );
   try
   {
      while( cursor.next() ) modes.push( cursor.current().toObj() );
   }
   finally
   {
      cursor.close();
   }
   return modes;
}

function drAssertNoGroupModes ( groupName )
{
   var modes = drListGroupModes( groupName );
   var activeModes = [];
   for( var i = 0; i < modes.length; ++i )
   {
      var properties = modes[i].Properties || [];
      if( modes[i].GroupMode !== undefined || properties.length !== 0 )
         activeModes.push( modes[i] );
   }
   if( activeModes.length !== 0 )
      throw new Error( "Expected no active GroupMode entries, actual: " +
         JSON.stringify( activeModes ) );
}

function drAssertGroupModeLocation ( groupName, mode, expectedLocation )
{
   var nodeLocations = {};
   var groupCursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName } );
   try
   {
      if( !groupCursor.next() )
         throw new Error( "Replica group not found: " + groupName );
      var nodes = groupCursor.current().toObj().Group || [];
      for( var i = 0; i < nodes.length; ++i )
      {
         var services = nodes[i].Service || [];
         for( var j = 0; j < services.length; ++j )
         {
            if( services[j].Type === 0 || services[j].Type === "0" )
              nodeLocations[String( nodes[i].HostName ) + ":" +
                 String( services[j].Name )] = String( nodes[i].Location );
         }
      }
   }
   finally
   {
      groupCursor.close();
   }

   var targets = [];
   var modeCursor = db.list( SDB_LIST_GROUPMODES,
      { GroupName: groupName, GroupMode: mode } );
   try
   {
      while( modeCursor.next() )
      {
         var properties = modeCursor.current().toObj().Properties || [];
         for( var p = 0; p < properties.length; ++p )
         {
            var target = properties[p].Location;
            if( target === undefined && properties[p].NodeName !== undefined )
               target = nodeLocations[String( properties[p].NodeName )];
            targets.push( target );
         }
      }
   }
   finally
   {
      modeCursor.close();
   }

   if( targets.length === 0 )
      throw new Error( "No " + mode + " target found for " + groupName );
   for( var t = 0; t < targets.length; ++t )
      if( targets[t] !== expectedLocation )
         throw new Error( "Unexpected " + mode + " target for " +
            groupName + ": expected " + expectedLocation + ", actual " +
            JSON.stringify( targets ) );
}

function drAssertGroupModeHost ( groupName, mode, expectedHost,
   expectedNodeCount )
{
   var targets = [];
   var directHostTarget = false;
   var cursor = db.list( SDB_LIST_GROUPMODES,
      { GroupName: groupName, GroupMode: mode } );
   try
   {
      while( cursor.next() )
      {
         var properties = cursor.current().toObj().Properties || [];
         for( var i = 0; i < properties.length; ++i )
         {
            if( properties[i].HostName !== undefined )
            {
               targets.push( String( properties[i].HostName ) );
               directHostTarget = true;
            }
            else if( properties[i].NodeName !== undefined )
            {
               targets.push( String( properties[i].NodeName ).split( ":" )[0] );
            }
         }
      }
   }
   finally
   {
      cursor.close();
   }

   if( targets.length === 0 )
      throw new Error( "No " + mode + " Host target found for " + groupName );
   for( var t = 0; t < targets.length; ++t )
      if( targets[t] !== expectedHost )
         throw new Error( "Unexpected " + mode + " Host target for " +
            groupName + ": expected " + expectedHost + ", actual " +
            JSON.stringify( targets ) );
   if( !directHostTarget && expectedNodeCount !== undefined &&
      targets.length !== expectedNodeCount )
      throw new Error( "Unexpected number of " + mode +
         " nodes for Host " + expectedHost + ": expected " +
         expectedNodeCount + ", actual " + targets.length );
}

function drBuildLocationFile ( plan, activeLocation )
{
   var content = "";
   var locations = Object.keys( plan ).sort();
   for( var i = 0; i < locations.length; ++i )
   {
      var location = locations[i];
      content += "[" + location +
         ( location === activeLocation ? "(active)" : "" ) + "]\n";
      for( var h = 0; h < plan[location].length; ++h )
         content += plan[location][h] + "\n";
      content += "\n";
   }
   return content;
}

function drCreateLocationGroup ( groupName, locations, nodesPerLocation )
{
   if( locations.length === 0 )
      throw new Error( "At least one Location name is required" );
   if( nodesPerLocation === undefined ) nodesPerLocation = 1;
   if( nodesPerLocation < 1 )
      throw new Error( "nodesPerLocation must be positive" );

   var nodeCount = locations.length * nodesPerLocation;
   var hostMap = {};
   var state = drCaptureLocationState();
   for( var s = 0; s < state.nodes.length; ++s )
      hostMap[state.nodes[s].hostName] = true;
   var hosts = Object.keys( hostMap ).sort();
   if( hosts.length === 0 ) throw new Error( "No cluster Host is available" );

   var nodes;
   if( hosts.length === 1 )
   {
      var originalNodeDir = RSRVNODEDIR;
      try
      {
         RSRVNODEDIR = drWorkDir + "nodes/";
         nodes = commCreateRG( db, groupName, nodeCount, hosts[0] );
      }
      finally
      {
         RSRVNODEDIR = originalNodeDir;
      }
   }
   else
   {
      nodes = [];
      var rg = db.createRG( groupName );
      var nextPort = parseInt( RSRVPORTBEGIN, 10 );
      var nodeRoot = String( RSRVNODEDIR );
      if( nodeRoot.charAt( nodeRoot.length - 1 ) !== "/" ) nodeRoot += "/";
      for( var i = 0; i < nodeCount; ++i )
      {
         // Keep nodes of the same Location on one Host so -H tests can target >=2 nodes.
         var hostName = hosts[Math.floor( i / nodesPerLocation ) % hosts.length];
         var created = false;
         while( !created && nextPort <= parseInt( RSRVPORTEND, 10 ) )
         {
            var serviceName = nextPort;
            nextPort += 10;
            var dbPath = nodeRoot + "data/" + serviceName;
            try
            {
               rg.createNode( hostName, serviceName, dbPath,
                  { diaglevel: 5 } );
               nodes.push( { hostname: hostName, svcname: serviceName } );
               created = true;
            }
            catch( e )
            {
               if( !commCompareErrorCode( e, SDBCM_NODE_EXISTED ) &&
                  !commCompareErrorCode( e, SDB_DIR_NOT_EMPTY ) )
                  throw e;
            }
         }
         if( !created ) throw new Error( "Reserved node ports exhausted" );
      }
      rg.start();
   }

   commCheckBusinessStatus( db, 120, true );
   var group = db.getRG( groupName );
   for( var n = 0; n < nodes.length; ++n )
      group.getNode( nodes[n].hostname, nodes[n].svcname ).setLocation(
         locations[Math.floor( n / nodesPerLocation )] );
   group.setActiveLocation( locations[0] );
   return nodes;
}

function drCreateSixNodeLocationGroup ( groupName, locations )
{
   if( locations.length !== 3 )
      throw new Error( "Three Location names are required" );
   return drCreateLocationGroup( groupName, locations, 2 );
}

function drReplicaGroupExists ( groupName )
{
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName } );
   try
   {
      return cursor.next();
   }
   finally
   {
      cursor.close();
   }
}

function drStopGroupModes ( groupName )
{
   if( !drReplicaGroupExists( groupName ) ) return;
   var rg = db.getRG( groupName );
   try
   {
      rg.stopMaintenanceMode();
   }
   catch( e )
   {
   }
   try
   {
      rg.stopCriticalMode();
   }
   catch( e )
   {
   }
}

function drDropReplicaGroup ( groupName )
{
   if( !drReplicaGroupExists( groupName ) ) return;
   drStopGroupModes( groupName );
   // Do not drop COMMCSNAME or scan/drop arbitrary CHANGEDPREFIX CS here:
   // both can be owned by the framework or another concurrent task.
   // The helper owns only the temporary replica group it created.
   db.removeRG( groupName );
}
