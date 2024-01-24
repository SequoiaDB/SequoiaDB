/*******************************************************************
* @Description : test export with timestamp data
*                seqDB-13509:集合中已存在所有数据类型的数据以及边界值，
*                            导出所有数据                 
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13509_timestamp";
var clname1 = COMMCLNAME + "_sdbimprt13509_timestamp";
var key = "timestamp";
var intMin = -2147483648;
var intMax = 2147483647;
var docs = [{ "timestamp": Timestamp( "1902-01-01-00:00:00.000000" ) },
{ "timestamp": Timestamp( "2037-12-31-23:59:59.999999" ) },
//  { "timestamp": Timestamp( "1902-01-01T00:00:00.000Z" ) },
//  { "timestamp": Timestamp( "2037-12-31T23:59:59.999Z" ) },
//  { "timestamp": Timestamp( "1902-01-01T00:00:00.000+0800" ) },
{ "timestamp": Timestamp( "2037-12-31T23:59:59.999+0800" ) },
{ "timestamp": Timestamp( intMin, 0 ) },
{ "timestamp": Timestamp( intMax, 0 ) }];
var csvContent = key + "\n" +
   "\"1902-01-01-00.00.00.000000\"\n" +
   "\"2037-12-31-23.59.59.999999\"\n" +
   //   "\"1902-01-01-08.05.52.000000\"\n" +
   //   "\"2038-01-01-07.59.59.999000\"\n" +
   //   "\"1902-01-01-00.05.52.000000\"\n" +
   "\"2037-12-31-23.59.59.999000\"\n" +
   "\"" + ms2ts( intMin ) + "\"\n" +
   "\"" + ms2ts( intMax ) + "\"\n";
var jsonContent =
   "{ \"" + key + "\": { \"$timestamp\": \"1902-01-01-00.00.00.000000\" } }\n" +
   "{ \"" + key + "\": { \"$timestamp\": \"2037-12-31-23.59.59.999999\" } }\n" +
   // "{ \"" + key + "\": { \"$timestamp\": \"1902-01-01-08.05.52.000000\" } }\n" +
   // "{ \"" + key + "\": { \"$timestamp\": \"2038-01-01-07.59.59.999000\" } }\n" +
   // "{ \"" + key + "\": { \"$timestamp\": \"1902-01-01-00.05.52.000000\" } }\n" +
   "{ \"" + key + "\": { \"$timestamp\": \"2037-12-31-23.59.59.999000\" } }\n" +
   "{ \"" + key + "\": { \"$timestamp\": \"" + ms2ts( intMin ) + "\" } }\n" +
   "{ \"" + key + "\": { \"$timestamp\": \"" + ms2ts( intMax ) + "\" } }\n";
var csvRecs = [
   "{\"" + key + "\":{\"$timestamp\":\"1902-01-01-00.00.00.000000\"}}",
   "{\"" + key + "\":{\"$timestamp\":\"2037-12-31-23.59.59.999999\"}}",
   // "{\"" + key + "\":{\"$timestamp\":\"1902-01-01-08.05.52.000000\"}}",
   // "{\"" + key + "\":{\"$timestamp\":\"2038-01-01-07.59.59.999000\"}}",
   // "{\"" + key + "\":{\"$timestamp\":\"1902-01-01-00.05.52.000000\"}}",
   "{\"" + key + "\":{\"$timestamp\":\"2037-12-31-23.59.59.999000\"}}",
   "{\"" + key + "\":{\"$timestamp\":\"" + ms2ts( intMin ) + "\"}}",
   "{\"" + key + "\":{\"$timestamp\":\"" + ms2ts( intMax ) + "\"}}"
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
   var csvfile = tmpFileDir + "sdbexprt13509_timestamp.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --fields " + key +
      " --type csv" +
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
      " --fields='" + key + " timestamp'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtImprtJson ()
{
   var jsonfile = tmpFileDir + "sdbexprt13509_timestamp.json";
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

// millseconds to timestamp
function ms2ts ( ms )
{
   var command = "date -d@\"" + ms +
      "\" +\"%Y-%m-%d-%H.%M.%S.000000\"";
   return cmd.run( command ).split( "\n" )[0];
}