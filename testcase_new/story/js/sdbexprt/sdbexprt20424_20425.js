/***************************************************************************
@Description : seqDB-20424:sdbexprt指定--hosts执行导出
               seqDB-20425:sdbexprt csv指定--field参数写成多行执行导出
@Modify list :
2020-01-15  chensiqin  Create
****************************************************************************/

testConf.csName = COMMCSNAME;
testConf.clName = COMMCLNAME + "_20424";

main( test );

function test ( testPara )
{

   cl = testPara.testCL;
   clName = testConf.clName
   cl.insert( { a: 1, b: 2 } );
   var csvDir = tmpFileDir + "20424/";
   cmd.run( "mkdir -p " + csvDir );
   testExprtCsv( csvDir, clName );
   testExprtCsv20425( csvDir, clName )
   testExprtJson( csvDir, clName );
   cmd.run( "rm -rf " + csvDir );
}

function testExprtCsv ( csvDir, clName )
{
   var csvContent = "a,b\n1,2\n";
   var filename = csvDir + "/" + COMMCSNAME + "." + "20425.csv";
   var command = installPath + "bin/sdbexprt" +
      " --hosts " + COORDHOSTNAME + ":" + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName +
      " --type csv " +
      "--fields a,b " +
      "--file " + filename;
   testRunCommand( command );
   checkFileContent( filename, csvContent );
}

function testExprtCsv20425 ( csvDir, clName )
{
   var csvContent = "a,b\n1,2\n";
   var filename = csvDir + "/" + COMMCSNAME + "." + clName + ".csv";
   var command = installPath + "bin/sdbexprt" +
      " --hosts " + COORDHOSTNAME + ":" + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName +
      " --type csv " +
      "--fields='a,\nb' " +
      "--file " + filename;
   testRunCommand( command );
   checkFileContent( filename, csvContent );
}

function testExprtJson ( csvDir, clName )
{
   var jsonContent = "{ \"a\": 1, \"b\": 2 }\n";
   var filename = csvDir + "/" + COMMCSNAME + "." + clName + ".json";
   var command = installPath + "bin/sdbexprt" +
      " --hosts " + COORDHOSTNAME + ":" + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName +
      " --type json " +
      "--fields a,b " +
      "--file " + filename;
   testRunCommand( command );
   checkFileContent( filename, jsonContent );
}