/*******************************************************************
* @Description : test export with bindata data
*                seqDB-13509:集合中已存在所有数据类型的数据以及边界值，
*                            导出所有数据                 
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13509_binary";
var clname1 = COMMCLNAME + "_sdbimprt13509_binary";
var key = "bindata";
var longStr = makeString( 1024 * 1024, 'x' );
var docs = [{ "bindata": BinData( "aGVsbG8gd29ybGQ=", "1" ) },
{ "bindata": BinData( "aGVsbG8gd29ybGQ=", "255" ) },
{ "bindata": { $binary: longStr, $type: "122" } }];
var csvContent = key + "\n" +
   "\"(1)aGVsbG8gd29ybGQ=\"\n" +
   "\"(255)aGVsbG8gd29ybGQ=\"\n" +
   "\"(122)" + longStr + "\"\n";
var jsonContent =
   "{ \"" + key + "\": { \"$binary\": \"aGVsbG8gd29ybGQ=\", \"$type\" : \"1\" } }\n" +
   "{ \"" + key + "\": { \"$binary\": \"aGVsbG8gd29ybGQ=\", \"$type\" : \"255\" } }\n" +
   "{ \"" + key + "\": { \"$binary\": \"" + longStr + "\", \"$type\" : \"122\" } }\n";
var csvRecs = [
   "{\"" + key + "\":{\"$binary\":\"aGVsbG8gd29ybGQ=\",\"$type\":\"1\"}}",
   "{\"" + key + "\":{\"$binary\":\"aGVsbG8gd29ybGQ=\",\"$type\":\"255\"}}",
   "{\"" + key + "\":{\"$binary\":\"" + longStr + "\",\"$type\":\"122\"}}"
];
var jsonRecs = csvRecs;

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );

   cl.insert( docs );

   testExprtImprtCsv();
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   var recs = getRecords( cursor );
   checkRecords( csvRecs, recs );
   cl1.truncate();

   testExprtImprtJson();
   cursor = cl1.find( {}, { _id: { $include: 0 } } );
   recs = getRecords( cursor );
   checkRecords( jsonRecs, recs );

   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}

function testExprtImprtCsv ()
{
   var csvfile = tmpFileDir + "sdbexprt13509_binary.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --fields " + key +
      " --type csv" +
      " --includebinary true" +
      " --sort '{ _id: 1 }'" +
      " --file " + csvfile;
   testRunCommand( command );

   checkFileContent( csvfile, csvContent );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --file " + csvfile +
      " --type csv" +
      " --headerline true" +
      " --fields='" + key + " binary'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtImprtJson ()
{
   var jsonfile = tmpFileDir + "sdbexprt13509_binary.json";
   cmd.run( "rm -rf " + jsonfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --type json" +
      " --fields " + key +
      " --sort '{ _id: 1 }'" +
      " --file " + jsonfile;
   testRunCommand( command );

   checkFileContent( jsonfile, jsonContent );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --type json" +
      " --file " + jsonfile +
      " --parsers 1 -j 1";
   testRunCommand( command );

   cmd.run( "rm -rf " + jsonfile );
}