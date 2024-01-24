/****************************************************************
@decription:   chang node conf by node.conf
@input:        hostname,   eg: 'localhost'
               svcname,    eg: 11810  
               mode,       'before' or 'after'        
@author:       Ting YU 2017-03-06   
****************************************************************/

if( typeof( hostname ) === "undefined" ) 
{ 
   throw "invalid para: hostname, can not be null"; 
}
if( typeof( svcname ) === "undefined" ) 
{ 
   throw "invalid para: svcname, can not be null"; 
}
if( mode !== "before" && mode !== "after" ) 
{ 
   throw 'invalid para: mode, should be "before" or "after"'; 
}
if( typeof( catalogConf ) === "undefined" )     var catalogConf = {}; 
if( typeof( catalogDynaConf ) === "undefined" ) var catalogDynaConf = {}; 
if( typeof( coordConf ) === "undefined" )       var coordConf = {}; 
if( typeof( coordDynaConf ) === "undefined" )   var coordDynaConf = {};
if( typeof( dataConf ) === "undefined" )        var dataConf = {}; 
if( typeof( dataDynaConf ) === "undefined" )    var dataDynaConf = {};
if( typeof( standaloneConf ) === "undefined" )  var standaloneConf = {}; 
if( typeof( standaloneConf ) === "undefined" )  var standaloneDynaConf = {};

var hasError = false;
var hostList = [];

main();

function main()
{  
   getHostList();
   
   if( mode === "before" )
   {
      getAllInitConf();

      setAllNodeConf( "static" );
      staticTakeEffect();
      setAllNodeConf( "dynamic" );
      dynamicTakeEffect();
      
      checkAllConfChange();
      checkNodeStart();
      checkPrimary();
      checkNodeHealth();
   }
   if( mode === "after" )
   {
      setAllNodeConf( "static" );
      staticTakeEffect();
      setAllNodeConf( "dynamic" );
      dynamicTakeEffect();

      checkAllConfChange();
      checkNodeStart();
      checkPrimary();
      checkNodeHealth();
   }
   
   if( hasError === true )
   {
      throw "There are errors in check list!"
   }
   else
   {
      println( "---executed successfully" );
   }
}

function getHostList()
{
   if( isStandalone() === false )
   {
      var db = new Sdb( hostname, svcname );
      var rc = db.list(7);
      while( rc.next() )
      {
         var groupInfo = rc.current().toObj().Group;
         for( var i in groupInfo )
         {
            hostList.push( groupInfo[i].HostName );
         }
      }
      hostList = uniqueArr( hostList );
      db.close();
   }
   else
   {
      hostList.push( hostname );
   }
   
   println( "---get hostlist: " + hostList );
}

function isStandalone()
{
   var isStand = false;
   try
   {
      var db = new Sdb( hostname, svcname );
      db.list(7);
   }
   catch(e)
   {
      if( e === -159 )
      {
         isStand = true;
      }
      else
      {
         throw e;
      }
   }
   return isStand;
}

function uniqueArr( arr )
{
   var tmp1 = [];
   for( var i in arr )
   {
      tmp1[arr[i]] = 1;
   }
   var tmp2 = [];
   for( var j in tmp1 )
   {
      tmp2.push( j );
   }
   return tmp2;
}

function getAllInitConf()
{
   println( '---begin to get init configure' );
   
   if( File.exist('./node.conf.ini') ) File.remove('./node.conf.ini');
   var initFile = new File( "./node.conf.ini" );
   getInitConf( "catalog", initFile );
   getInitConf( "coord", initFile );
   getInitConf( "data", initFile );
   getInitConf( "standalone", initFile );
   initFile.close();
}

function getInitConf( role, initFile )
{
   // get first catalog node
   for( var i in hostList )
   {
      var oma = new Oma( hostList[i], 11790 );
      var nodes = oma.listNodes( { role: role } ).toArray();

      if( nodes.length > 0 )
      {
         var nodeInfoStr = nodes[0];
         var nodeInfoObj = JSON.parse( nodeInfoStr );
         var portStr = nodeInfoObj.svcname;

         var nodeInfoArr = oma.listNodes( { svcname: portStr, expand: true } ).toArray();
         var nodeConf = JSON.parse( nodeInfoArr[0] );
         break;
      }
   }
   
   if( typeof( nodeConf ) === "undefined" ) return;
   
   switch( role )
   {
      case "catalog":
         var conf = catalogConf;
         var dynaConf = catalogDynaConf;
         break;
      case "coord":
         var conf = coordConf;
         var dynaConf = coordDynaConf;
         break;
      case "data":
         var conf = dataConf;
         var dynaConf = dataDynaConf;
         break;
      case "standalone":
         var conf = standaloneConf;
         var dynaConf = standaloneDynaConf;
         break;
      default:
         break;
   }
   
   var confInit = {};
   for( var f in conf )
   {  
      var initValue = nodeConf[f];
      confInit[f] = initValue;
   }

   var dynaConfInit = {};
   for( var f in dynaConf )
   {
      var initValue = nodeConf[f];
      dynaConfInit[f] = initValue;
   }

   initFile.write( role + "Conf=" + JSON.stringify( confInit ) + "\n" );
   initFile.write( role + "DynaConf=" + JSON.stringify( dynaConfInit ) + "\n" );
}

// check restart of all node
function checkNodeStart()
{
   println( '---begin to check nodes restarted' );
   
   for( var i in hostList )
   {
      var oma = new Oma( hostList[i], 11790 );
      var localNodes = oma.listNodes( { mode: 'local' } ).toArray();

      for( var j in localNodes )
      {
         var nodeInfoStr = localNodes[j];
         var nodeInfoObj = JSON.parse( nodeInfoStr );
         var port = nodeInfoObj.svcname;
         var activeNodes = oma.listNodes( { mode: 'run', svcname: port } ).toArray();
         if( activeNodes.length === 0 )
         {
            println( "node[" + hostList[i] + ':' + port + "] is not active" );
            hasError = true;
         }
      }
   }
}

function checkNodeHealth()
{
   println( '---begin to check nodes health' );
   var db = new Sdb( hostname, svcname  );
   var hasErrorNode = false;
   for(var i = 0; i < 6000; i++ )
   {
      hasErrorNode = false;
      var localNodes = db.snapshot( SDB_SNAP_HEALTH, new SdbSnapshotOption().sel({ServiceStatus:1,Status:1,NodeName:1}) ).toArray();
      for( var j in localNodes )
      {
         var nodeInfoStr = localNodes[j];
         var nodeInfoObj = JSON.parse( nodeInfoStr );
         if( nodeInfoObj.ServiceStatus != true || nodeInfoObj.Status != "Normal" )
         {
            println( nodeInfoObj.NodeName + " ServiceStatus is " + nodeInfoObj.ServiceStatus + " Status is" + nodeInfoObj.Status);
            hasErrorNode = true;
         }
      }
      if( hasErrorNode === true )
      {
         sleep(100); 
      }
      else
      {
         break;
      }
   }
   if( hasErrorNode === true )
   {
      hasError = true;
   }
}

// check primary of all groups, except coord
function checkPrimary()
{
   println( '---begin to check all groups have primary' );
   
   var db = new Sdb( hostname, svcname  );
   var datargList = getDatargName( db );
   for( var i in datargList )
   {  
      checkOneGroupPrimary( db, datargList[i] );
   }
   checkOneGroupPrimary( db, "SYSCatalogGroup" );
}

function checkOneGroupPrimary( db, groupName )
{
   var hasPrimary = false;
   
   // wait group to select primary for 60000ms
   for(var i = 0; i < 6000; i++ )
   {  
      hasPrimary = false;
      // check primary at once
      var rc = db.exec( "select NodeName,IsPrimary from $SNAPSHOT_SYSTEM where GroupName='" + groupName + "'" );
      while( rc.next() )
      {
         var fieldIsPrimary = rc.current().toObj().IsPrimary;
         if( fieldIsPrimary === true )
         {
            hasPrimary = true;
            break;
         }
      }
      rc.close();
      
      if( hasPrimary === false )
      {
         sleep(100); 
      }
      else
      {
         break;
      }
   }

   if( hasPrimary === false )
   {
      println( "there is no primary node in group: " + groupName );
      hasError = true;
   }
}

// check all node configure have been changed or not
function checkAllConfChange()
{
   println( '---begin to check conf file changed' );
   
   // every host
   for( var i in hostList )
   {
      var oma = new Oma( hostList[i], 11790 );
      var nodeInfoArr = oma.listNodes().toArray();
      
      // every node
      for( var k in nodeInfoArr )
      {
         var nodeInfoStr = nodeInfoArr[k];
         var nodeInfoObj = JSON.parse( nodeInfoStr );
         checkConfChange( oma, nodeInfoObj.role, nodeInfoObj.svcname );//TODO how to print error
      }

      oma.close();
   }
}

// check a node configure has been changed or not
function checkConfChange( oma, role, svcname )
{  
   var nodeObj = oma.getNodeConfigs( svcname ).toObj();
   switch( role )
   {  
      case "catalog":
         compareTwoObj( nodeObj, catalogConf );
         compareTwoObj( nodeObj, catalogDynaConf );
         break;
      case "coord":
         compareTwoObj( nodeObj, coordConf );
         compareTwoObj( nodeObj, coordDynaConf );
         break;
      case "data":
         compareTwoObj( nodeObj, dataConf );
         compareTwoObj( nodeObj, dataDynaConf );
         break;
      case "standalone":
         compareTwoObj( nodeObj, standaloneConf );
         compareTwoObj( nodeObj, standaloneDynaConf );
         break;
      default:
         break;
   }
}

// TODO
function compareTwoObj( actObj, expObj )
{
   for( var i in expObj )
   {
      if( actObj[i].toString().toUpperCase() !== expObj[i].toString().toUpperCase())
      {
         println(     'expect: ' + i + '=' + expObj[i]
                  + ', actual: ' + i + '=' + actObj[i] );
         hasError = true;
      }
   }
}

// restart nodes to take effect
function staticTakeEffect()
{
   if( isEmptyObj( coordConf )      === false ) restartNodes( "coord" );
   if( isEmptyObj( catalogConf )    === false ) restartNodes( "catalog" );
   if( isEmptyObj( dataConf )       === false ) restartNodes( "data" );
   if( isEmptyObj( standaloneConf ) === false ) restartNodes( "standalone" );
}

function restartNodes( role )
{
   // every host
   for( var i in hostList )
   {
      var oma = new Oma( hostList[i], 11790 );
      var nodeInfoArr = oma.listNodes( { role: role, mode:"local", expand: true } ).toArray();
      
      // every node
      for( var k in nodeInfoArr )
      {
         var nodeInfoStr = nodeInfoArr[k];
         var nodeInfoObj = JSON.parse( nodeInfoStr );
         
         println( '-----begin to restart ' + role + ' node[' + hostList[i] + ':' + nodeInfoObj.svcname + ']');
         oma.stopNode( nodeInfoObj.svcname );
         oma.startNode( nodeInfoObj.svcname );
         markRestart( hostList[i], nodeInfoObj.diagpath )
      }
      oma.close();
   }
}

function markRestart( hostname, diagpath )
{
   println('-------begin to mark '+hostname+' restart '+diagpath);
   var remote = new Remote( hostname, 11790 );
   
   var cmd = remote.getCmd();
   var currentTime = cmd.run( 'date "+%Y-%m-%d-%H.%M.%S"' );
   
   var file = remote.getFile( diagpath + "/.RESTART_BY_CHANGECONF" );
   file.seek( 0, 'e' );
   file.write( "restart by tool changeConf.js at " + currentTime );
   file.close();
}

// reload dynamic conf to take effect
function dynamicTakeEffect()
{
   println( '---begin to make dynamic configure effect' );
   
   if(    isEmptyObj( catalogDynaConf )  === false 
       || isEmptyObj( coordDynaConf ) === false 
       || isEmptyObj( dataDynaConf )  === false 
       || isEmptyObj( standaloneDynaConf )  === false )
   {
      var db = new Sdb( hostname, svcname  );
      println( '-----begin to reload configure by node[' + hostname + ':' + svcname + ']' );
      db.reloadConf();
   }
}

// set configure of all nodes, static or dynamic
function setAllNodeConf( confType )
{
   println( '---begin to set ' + confType + ' configure' );
   
   // every host
   for( var i in hostList )
   {
      var oma = new Oma( hostList[i], 11790 );      
      var nodeInfoArr = oma.listNodes( { mode: "local" } ).toArray();
      
      // every node
      for( var k in nodeInfoArr )
      {
         var nodeInfoStr = nodeInfoArr[k];
         var nodeInfoObj = JSON.parse( nodeInfoStr );
         switch( confType )
         {
            case "static":
               setStaticConf( oma, nodeInfoObj.role, nodeInfoObj.svcname );
               break;
            case "dynamic":
               setDynaConf( oma, nodeInfoObj.role, nodeInfoObj.svcname);
               break;
         }
      }

      oma.close();
   }
}

// set static configure of one node
function setStaticConf( oma, role, svcname )
{
   switch( role )
   {
      case "catalog":
         if( isEmptyObj( catalogConf ) === false )
         {
            oma.updateNodeConfigs( svcname, catalogConf );
         }
         break;
         
      case "coord":
         if( isEmptyObj( coordConf ) === false )
         {
            oma.updateNodeConfigs( svcname, coordConf );
         }
         break;  
         
      case "data":
         if( isEmptyObj( dataConf ) === false )
         {
            oma.updateNodeConfigs( svcname, dataConf );
         }
         break;
         
      case "standalone":
         if( isEmptyObj( standaloneConf ) === false )
         {
            oma.updateNodeConfigs( svcname, standaloneConf );
         }
         break;
      
      default:
         break;
   }
}

// set dynamic configure of one node
function setDynaConf( oma, role, svcname )
{
   
   switch( role )
   {
      case "catalog":
         if( isEmptyObj( catalogDynaConf ) === false )
         {
            oma.updateNodeConfigs( svcname, catalogDynaConf );
         }
         break;
         
      case "coord":
         if( isEmptyObj( coordDynaConf ) === false )
         {
            oma.updateNodeConfigs( svcname, coordDynaConf );
         }
         break;  
         
      case "data":
         if( isEmptyObj( dataDynaConf ) === false )
         {
            println("222setDynaConf :" + role);
            oma.updateNodeConfigs( svcname, dataDynaConf );
         }
         break;
         
      case "standalone":
         if( isEmptyObj( standaloneDynaConf ) === false )
         {
            oma.updateNodeConfigs( svcname, standaloneDynaConf );
         }
         break;
      
      default:
         break;
   }
}

// get all data group name
function getDatargName( db )
{
   var datargArr = [];
   
   var rc = db.list( 7, { Role: 0 } );
   while( rc.next() )
   {
      var groupName = rc.current().toObj().GroupName;
      datargArr.push( groupName );
   }
   
   return datargArr;
}

// is empty object: {}
function isEmptyObj( obj )
{
   for( var key in obj )
   {
      return false;
   }
   return true;
}
