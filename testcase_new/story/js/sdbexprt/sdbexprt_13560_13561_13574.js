/*******************************************************************
* @Description : seqDB-13560:--cscl指定多个集合导出数据到json
*                seqDB-13561:--cscl指定多个集合导出数据到csv
*                seqDB-13574:--dir导出数据到指定目录                
* @author      : Liang XueWang 
*
*******************************************************************/
var clNum = 5;
var clNames = [];
var csNames = [];
var csvContent = "a\n1\n";
var jsonContent = "{ \"_id\": 1, \"a\": 1 }\n"

main( test );

function test ()
{
   for( var i = 0; i < clNum; i++ )
   {
      var csName = COMMCSNAME + "_sdbexprt13560_" + i;
      var clName = COMMCLNAME + "_sdbexprt13560_" + i;
      var cl = commCreateCL( db, csName, clName );
      cl.insert( { "_id": 1, "a": 1 } );
      clNames.push( clName );
      csNames.push( csName );
   }

   testExprtCsv();
   testExprtJson();

   for( var i = 0; i < clNum; i++ )
   {
      commDropCS( db, csNames[i] );
   }
}

function testExprtCsv ()
{
   var csvDir = tmpFileDir + "13561/";
   cmd.run( "mkdir -p " + csvDir );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --dir " + csvDir +
      " --force true" +
      " --type csv";
   command += " --cscl ";
   for( var i = 0; i < clNum; i++ )
   {
      command += csNames[i] + "." + clNames[i];
      if( i !== clNum - 1 )
         command += ",";
   }
   testRunCommand( command );

   for( var i = 0; i < clNum; i++ )
   {
      var fileName = csvDir + csNames[i] + "." + clNames[i] + ".csv";
      checkFileContent( fileName, csvContent );
   }

   cmd.run( "rm -rf " + csvDir );
}

function testExprtJson ()
{
   var jsonDir = tmpFileDir + "13560/";
   cmd.run( "mkdir -p " + jsonDir );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --dir " + jsonDir +
      " --type json";
   command += " --cscl ";
   for( var i = 0; i < clNum; i++ )
   {
      command += csNames[i] + "." + clNames[i];
      if( i !== clNum - 1 )
         command += ",";
   }
   testRunCommand( command );

   for( var i = 0; i < clNum; i++ )
   {
      var fileName = jsonDir + csNames[i] + "." + clNames[i] + ".json";
      checkFileContent( fileName, jsonContent );
   }

   cmd.run( "rm -rf " + jsonDir );
}
