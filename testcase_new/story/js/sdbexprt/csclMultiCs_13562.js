/*******************************************************************
* @Description : test export with --cscl multi cs
*                seqDB-13562:--cscl指定多个集合空间导出数据到json
*                seqDB-13563:--cscl指定多个集合空间导出数据到csv              
* @author      : Liang XueWang 
*
*******************************************************************/
var clnum = 5;
var clnames = [];
var csnames = [];
var doc = { a: 1 };
var csvContent = "a\n1\n";
var jsonContent = "{ \"a\": 1 }\n"

main( test );

function test ()
{
   for( var i = 0; i < clnum; i++ )
   {
      var csname = COMMCSNAME + "_sdbexprt13562_" + i;
      var clname = COMMCLNAME + "_sdbexprt13562_" + i;
      var cl = commCreateCL( db, csname, clname );
      cl.insert( doc );
      clnames.push( clname );
      csnames.push( csname );
   }

   testExprtCsv();
   testExprtJson();

   for( var i = 0; i < clnum; i++ )
   {
      commDropCS( db, csnames[i] );
   }
}

function testExprtCsv ()
{
   var csvDir = tmpFileDir + "13563/";
   cmd.run( "mkdir -p " + csvDir );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --dir " + csvDir +
      " --type csv";
   command += " --cscl ";
   for( var i = 0; i < clnum; i++ )
   {
      command += csnames[i];
      if( i !== clnum - 1 )
         command += ",";
   }
   for( var i = 0; i < clnum; i++ )
   {
      command += " --fields " + csnames[i] + "." + clnames[i] + ":a";
   }
   testRunCommand( command );

   for( var i = 0; i < clnum; i++ )
   {
      var filename = csvDir + csnames[i] + "." + clnames[i] + ".csv";
      checkFileContent( filename, csvContent );
   }

   cmd.run( "rm -rf " + csvDir );
}

function testExprtJson ()
{
   var jsonDir = tmpFileDir + "13562/";
   cmd.run( "mkdir -p " + jsonDir );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --dir " + jsonDir +
      " --type json";
   command += " --cscl ";
   for( var i = 0; i < clnum; i++ )
   {
      command += csnames[i];
      if( i !== clnum - 1 )
         command += ",";
   }
   for( var i = 0; i < clnum; i++ )
   {
      command += " --fields " + csnames[i] + "." + clnames[i] + ":a";
   }
   testRunCommand( command );

   for( var i = 0; i < clnum; i++ )
   {
      var filename = jsonDir + csnames[i] + "." + clnames[i] + ".json";
      checkFileContent( filename, jsonContent );
   }

   cmd.run( "rm -rf " + jsonDir );
}