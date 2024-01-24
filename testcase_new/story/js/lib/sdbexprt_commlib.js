import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );


var cmd = new Cmd();
var installPath = getInstallDir();
var tmpFileDir = WORKDIR + "/sdbexprt/";
readyTmpDir();

/* ***************************************************
@description : ��ȡbin/sdbexprt����Ŀ¼
@author: XiaoNi Huang 2019-12-19
**************************************************** */
function getInstallDir ()
{
   var localDir = cmd.run( "pwd" ).split( "\n" )[0] + "/";
   var installDir = '';

   try
   {
      cmd.run( 'find ./bin/sdbexprt' ).split( '\n' )[0];
      installDir = localDir;
   }
   catch( e ) 
   {
      installDir = commGetInstallPath() + "/";
   }

   return installDir;
}

/* ****************************************************
@description: ready tmp director
@author: XiaoNi Huang 2019-12-19
**************************************************** */
function readyTmpDir ()
{
   cmd.run( "rm -rf " + tmpFileDir );

   cmd.run( "mkdir -p " + tmpFileDir );
}

/*******************************************************************
* @Description : check host is localhost or local hostname
* @author      : Liang XueWang
*
********************************************************************/
function isLocal ( hostname )
{
   if( hostname === "localhost" )
      return true;
   if( hostname === cmd.run( "hostname" ).split( "\n" )[0] )
      return true;
   return false;
}

/*******************************************************************
* @Description : get groups, include SYSCoord and SYSCatalogGroup
*                return array like [ "group1", ... ]
* @author      : Liang XueWang
*
********************************************************************/
function getGroups ()
{
   var groups = [];
   if( commIsStandalone( db ) )
   {
      return groups;
   }
   var cursor = db.listReplicaGroups();
   var obj;
   while( obj = cursor.next() )
   {
      var groupName = obj.toObj()["GroupName"];
      groups.push( groupName );
   }
   return groups;
}

/*******************************************************************
* @Description : get group nodes, 
*                return array like [ "sdbserver1:11830", ... ]
* @author      : Liang XueWang
*
********************************************************************/
function getGroupNodes ( groupName )
{
   var nodes = [];
   if( commIsStandalone( db ) )
   {
      return nodes;
   }
   var rg = db.getRG( groupName );
   var obj = rg.getDetail().next().toObj();
   var groupArr = obj["Group"];
   for( var i = 0; i < groupArr.length; i++ )
   {
      var hostname = groupArr[i]["HostName"];
      var svcname = groupArr[i]["Service"][0]["Name"];
      nodes.push( hostname + ":" + svcname );
   }
   return nodes;
}

/*******************************************************************
* @Description : get a random int [m n)      
* @author      : Liang XueWang
*
********************************************************************/
function getRandomInt ( m, n )
{
   var range = n - m;
   var ret = m + parseInt( Math.random() * range );
   return ret;
}

/*******************************************************************
* @Description : insert record which use kilobytes space       
* @author      : Liang XueWang
*
********************************************************************/
function insertKBDocs ( cl, kb )
{
   var docs = [];

   for( var i = 0; i < parseInt( kb ); i++ )
   {
      var doc = {};
      doc["key"] = makeString( 1024, 'x' );
      doc["cnt"] = i + 1;
      docs.push( doc );
      if( docs.length % 10000 === 0 || i === parseInt( kb ) - 1 )
      {
         cl.insert( docs );
         docs = [];
      }
   }
}

/*******************************************************************
* @Description : test run command        
* @author      : Liang XueWang
*
********************************************************************/
function testRunCommand ( command, errno )
{
   if( errno == undefined )
   {
      cmd.run( command );
   } else
   {
      assert.tryThrow( errno, function()
      {
         cmd.run( command );
      } );
   }
}
/*******************************************************************
* @Description : check file content       
* @author      : Liang XueWang
*
********************************************************************/
function checkFileContent ( filename, expContent )
{
   var size = parseInt( File.stat( filename ).toObj().size );
   var file = new File( filename );
   var actContent = file.read( size );
   file.close();
   assert.equal( actContent, expContent );
}

/*******************************************************************
* @Description : get records from cursor, return record array     
* @author      : Liang XueWang
*
********************************************************************/
function getRecords ( cursor )
{
   var recs = [];
   var obj;
   while( obj = cursor.next() )
   {
      recs.push( JSON.stringify( obj.toObj() ) );
   }
   return recs;
}

/*******************************************************************
* @Description : check records       
* @author      : Liang XueWang
*
********************************************************************/
function checkRecords ( expRecs, actRecs )
{
   assert.equal( expRecs.length, actRecs.length );
   for( var i = 0; i < expRecs.length; i++ )
   {
      assert.equal( expRecs[i], actRecs[i] );
   }
}

/*******************************************************************
* @Description : check file exist  
* @author      : Liang XueWang
*
********************************************************************/
function checkFileExist ( filename, exist )
{
   var res = File.exist( filename );
   assert.equal( res, exist );
}

/*******************************************************************
* @Description : get current user
* @author      : Liang XueWang
*
********************************************************************/
function getCurrentUser ()
{
   return System.getCurrentUser().toObj()["user"];
}

/*******************************************************************
* @Description : make string with length len
* @author      : Liang XueWang
*
********************************************************************/
function makeString ( len, ch )
{
   var arr = new Array( len + 1 );
   return arr.join( ch );
}

/*******************************************************************
* @Description : remove rec file if needed
* @author      : Liang XueWang
*
********************************************************************/
function rmRecFile ( csname, clname )
{
   var files = csname + "_" + clname + "_*.rec";
   cmd.run( "rm -rf ./" + files );
}
