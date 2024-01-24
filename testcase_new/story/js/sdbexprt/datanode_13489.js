/*******************************************************************
* @Description : test export with data node -s -p
*                seqDB-13489:指定从数据节点导出数据                
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13489";
var doc = { a: 1 };
var csvContent = "a\n1\n";
var jsonContent = "{ \"a\": 1 }\n";

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var groups = getGroups();
   for( var i = 0; i < groups.length; i++ )
   {
      var groupName = groups[i];
      if( groupName !== "SYSCoord" && groupName !== "SYSCatalogGroup" )
         break;
   }
   var nodes = getGroupNodes( groupName );
   var host = nodes[0].split( ":" )[0];
   var svc = nodes[0].split( ":" )[1];

   var option = { Group: groupName, ReplSize: 0 };
   var cl = commCreateCL( db, csname, clname, option );
   cl.insert( doc );

   testExprtCsv( host, svc );
   testExprtJson( host, svc );

   commDropCL( db, csname, clname );
}

function testExprtCsv ( hostname, svcname )
{
   var csvfile = tmpFileDir + "sdbexprt13489.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + hostname +
      " -p " + svcname +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields a";
   testRunCommand( command );

   checkFileContent( csvfile, csvContent );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtJson ( hostname, svcname )
{
   var jsonfile = tmpFileDir + "sdbexprt13489.json";
   cmd.run( "rm -rf " + jsonfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + hostname +
      " -p " + svcname +
      " -c " + csname +
      " -l " + clname +
      " --type json" +
      " --file " + jsonfile +
      " --fields a";
   testRunCommand( command );

   checkFileContent( jsonfile, jsonContent );

   cmd.run( "rm -rf " + jsonfile );
}

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