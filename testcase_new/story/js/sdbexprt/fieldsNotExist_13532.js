/*******************************************************************
* @Description : test export with --fields different record
*                seqDB-13532:指定的字段名在部分记录存在，
*                            部分记录不存在，导出数据到json    
*                seqDB-13536:指定的字段名在部分记录存在，
*                            部分记录不存在，导出数据到csv          
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13532";
var clname1 = COMMCLNAME + "_sdbimprt13532";
var docs = [{ a: 1 }, { a: 2, b: 2, c: 3 }, { d: 4 }];
var csvContent = "a,b,c\n" +
   "1,,\n" +
   "2,2,3\n" +
   ",,\n";
var jsonContent = "{ \"a\": 1 }\n" +
   "{ \"a\": 2, \"b\": 2, \"c\": 3 }\n" +
   "{  }\n";
var csvRecs = ["{\"a\":1}",
   "{\"a\":2,\"b\":2,\"c\":3}",
   "{}"];
var jsonRecs = ["{\"a\":1}",
   "{\"a\":2,\"b\":2,\"c\":3}",
   "{}"];

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );
   cl.insert( docs );

   testExprtImprtCsv();
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   var actRecs = getRecords( cursor );
   checkRecords( csvRecs, actRecs );
   cl1.truncate();

   testExprtImprtJson();
   cursor = cl1.find( {}, { _id: { $include: 0 } } );
   actRecs = getRecords( cursor );
   checkRecords( jsonRecs, actRecs );

   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}

function testExprtImprtCsv ()
{
   var csvfile = tmpFileDir + "sdbexprt13532.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --sort '{ _id: 1 }'" +
      " --fields a,b,c";

   testRunCommand( command );

   checkFileContent( csvfile, csvContent );

   command = installPath + "bin/sdbimprt " +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --file " + csvfile +
      " --type csv " +
      " --headerline true" +
      " --fields='a int,b int,c int'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtImprtJson ()
{
   var jsonfile = tmpFileDir + "sdbexprt13532.json";
   cmd.run( "rm -rf " + jsonfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + jsonfile +
      " --type json" +
      " --sort '{ _id: 1 }'" +
      " --fields a,b,c";
   testRunCommand( command );

   checkFileContent( jsonfile, jsonContent );

   command = installPath + "bin/sdbimprt " +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --file " + jsonfile +
      " --type json" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   cmd.run( "rm -rf " + jsonfile );
}