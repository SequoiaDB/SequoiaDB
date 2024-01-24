/*******************************************************************
* @Description : seqDB-22276:多集合导出不包含_id的数据 
* @author      : Liang XueWang 
*******************************************************************/
var clNum = 5;
var clNames = [];
var csNames = [];

main( test );

function test ()
{
   for( var i = 0; i < clNum; i++ )
   {
      var csName = COMMCSNAME + "_sdbexprt22276_" + i;
      var clName = COMMCLNAME + "_sdbexprt22276_" + i;
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
   var csvContent = "a\n1\n";
   var csvDir = tmpFileDir + "22276/";
   cmd.run( "mkdir -p " + csvDir );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --dir " + csvDir +
      " --type csv" +
      " --withid false" +
      " --force true ";
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
   var jsonContent = "{ \"a\": 1 }\n"
   var jsonDir = tmpFileDir + "22276/";
   cmd.run( "mkdir -p " + jsonDir );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --dir " + jsonDir +
      " --withid false" +
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
