/*******************************************************************
* @Description : test export with regex data
*                seqDB-13509:集合中已存在所有数据类型的数据以及边界值，
*                            导出所有数据                 
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13509_regex";
var clname1 = COMMCLNAME + "_sdbimprt13509_regex";
var key = "regex";
var longStr = makeString( 1024 * 1024, 'x' );
var docs = [{ "regex": Regex( "^abc", "i" ) },
{ "regex": Regex( "abc$", "m" ) },
{ "regex": Regex( "^abc", "x" ) },
{ "regex": Regex( "abc$", "s" ) },
{ "regex": { $regex: longStr, $options: "x" } }];
var csvContent = key + "\n" +
   "\"/^abc/i\"\n" +
   "\"/abc$/m\"\n" +
   "\"/^abc/x\"\n" +
   "\"/abc$/s\"\n" +
   "\"/" + longStr + "/x\"\n";
var jsonContent =
   "{ \"" + key + "\": { \"$regex\": \"^abc\", \"$options\": \"i\" } }\n" +
   "{ \"" + key + "\": { \"$regex\": \"abc$\", \"$options\": \"m\" } }\n" +
   "{ \"" + key + "\": { \"$regex\": \"^abc\", \"$options\": \"x\" } }\n" +
   "{ \"" + key + "\": { \"$regex\": \"abc$\", \"$options\": \"s\" } }\n" +
   "{ \"" + key + "\": { \"$regex\": \"" + longStr + "\", \"$options\": \"x\" } }\n";
var csvRecs = [
   "{\"" + key + "\":{\"$regex\":\"^abc\",\"$options\":\"i\"}}",
   "{\"" + key + "\":{\"$regex\":\"abc$\",\"$options\":\"m\"}}",
   "{\"" + key + "\":{\"$regex\":\"^abc\",\"$options\":\"x\"}}",
   "{\"" + key + "\":{\"$regex\":\"abc$\",\"$options\":\"s\"}}",
   "{\"" + key + "\":{\"$regex\":\"" + longStr + "\",\"$options\":\"x\"}}"
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
   var csvfile = tmpFileDir + "sdbexprt13509_regex.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --fields " + key +
      " --type csv" +
      " --includeregex true" +
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
      " --fields='" + key + " regex'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtImprtJson ()
{
   var jsonfile = tmpFileDir + "sdbexprt13509_regex.json";
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