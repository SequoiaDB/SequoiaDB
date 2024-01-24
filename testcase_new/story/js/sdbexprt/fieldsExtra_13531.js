/*******************************************************************
* @Description : test export with --fields have not exist field
*                seqDB-13531:指定的多个字段中多了一个不存在的字段，
*                            导出数据到json
*                seqDB-13535:指定的多个字段中多了一个不存在的字段，
*                            导出数据到csv              
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13531";
var doc = { a: 1 };
var csvContent = "a,b\n1,\n";
var jsonContent = "{ \"a\": 1 }\n";

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   cl.insert( doc );

   testExprtCsv();
   testExprtJson();

   commDropCL( db, csname, clname );
}

function testExprtCsv ()
{
   var csvfile = tmpFileDir + "sdbexprt13531.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields a,b";

   testRunCommand( command );

   checkFileContent( csvfile, csvContent );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtJson ()
{
   var jsonfile = tmpFileDir + "sdbexprt13535.json";
   cmd.run( "rm -rf " + jsonfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + jsonfile +
      " --type json" +
      " --fields a,b";

   testRunCommand( command );

   checkFileContent( jsonfile, jsonContent );

   cmd.run( "rm -rf " + jsonfile );
}