/*******************************************************************
* @Description : test export with decimal data
*                seqDB-13509:集合中已存在所有数据类型的数据以及边界值，
*                            导出所有数据                 
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13509_decimal";
var clname1 = COMMCLNAME + "_sdbimprt13509_decimal";
var key = "decimal";
var longStr = makeString( 999, '0' );
var docs = [{ "decimal": { $decimal: "-9223372036854775809.001" } },
{ "decimal": { $decimal: "9223372036854775808.001" } },
{ "decimal": { $decimal: "0" } },
{ "decimal": { $decimal: "0.5" } },
{ "decimal": { $decimal: "1", $precision: [1, 0] } },
{ "decimal": { $decimal: "4", $precision: [1000, 999] } }];
var csvContent = key + "\n" +
   "-9223372036854775809.001\n" +
   "9223372036854775808.001\n" +
   "0\n" + "0.5\n" + "1\n" +
   "4." + longStr + "\n";
var jsonContent =
   "{ \"" + key + "\": { \"$decimal\": \"-9223372036854775809.001\" } }\n" +
   "{ \"" + key + "\": { \"$decimal\": \"9223372036854775808.001\" } }\n" +
   "{ \"" + key + "\": { \"$decimal\": \"0\" } }\n" +
   "{ \"" + key + "\": { \"$decimal\": \"0.5\" } }\n" +
   "{ \"" + key + "\": { \"$decimal\": \"1\", \"$precision\": [1, 0] } }\n" +
   "{ \"" + key + "\": { \"$decimal\": \"4." + longStr + "\", \"$precision\": [1000, 999] } }\n";
var csvRecs = [
   "{\"" + key + "\":{\"$decimal\":\"-9223372036854775809.001\"}}",
   "{\"" + key + "\":{\"$decimal\":\"9223372036854775808.001\"}}",
   "{\"" + key + "\":{\"$decimal\":\"0\"}}",
   "{\"" + key + "\":{\"$decimal\":\"0.5\"}}",
   "{\"" + key + "\":{\"$decimal\":\"1\"}}",
   "{\"" + key + "\":{\"$decimal\":\"4." + longStr + "\"}}"
];
var jsonRecs = [
   "{\"" + key + "\":{\"$decimal\":\"-9223372036854775809.001\"}}",
   "{\"" + key + "\":{\"$decimal\":\"9223372036854775808.001\"}}",
   "{\"" + key + "\":{\"$decimal\":\"0\"}}",
   "{\"" + key + "\":{\"$decimal\":\"0.5\"}}",
   "{\"" + key + "\":{\"$decimal\":\"1\",\"$precision\":[1,0]}}",
   "{\"" + key + "\":{\"$decimal\":\"4." + longStr + "\",\"$precision\":[1000,999]}}"
];

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
   var csvfile = tmpFileDir + "sdbexprt13509_decimal.csv";
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
      " --fields='" + key + " decimal'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtImprtJson ()
{
   var jsonfile = tmpFileDir + "sdbexprt13509_decimal.json";
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