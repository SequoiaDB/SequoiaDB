/*******************************************************************
* @Description : test export with --strict
*                seqDB-12236:sdbexprt导出json，指定--strict                 
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt12236";
var clname1 = COMMCLNAME + "_sdbimprt12236";
var key = "long";
var longMin = "-9223372036854775808";
var longMax = "9223372036854775807";
var intMin = "-2147483648";
var intMax = "2147483647";
var docs = [{ "long": NumberLong( longMin ) },
{ "long": NumberLong( intMin ) },
{ "long": NumberLong( intMax ) },
{ "long": NumberLong( longMax ) }];
var expRecs = [
   "{\"" + key + "\":{\"$numberLong\":\"" + longMin + "\"}}",
   "{\"" + key + "\":" + intMin + "}",
   "{\"" + key + "\":" + intMax + "}",
   "{\"" + key + "\":{\"$numberLong\":\"" + longMax + "\"}}"];

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );

   cl.insert( docs );

   testExprtStrict1();  // test strict true
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   var recs = getRecords( cursor );
   checkRecords( expRecs, recs );
   cl1.truncate();

   testExprtStrict2();  // test strict false
   cursor = cl1.find( {}, { _id: { $include: 0 } } );
   recs = getRecords( cursor );
   checkRecords( expRecs, recs );

   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}

function testExprtStrict1 ()
{
   var jsonfile = tmpFileDir + "sdbexprt12236.json";
   cmd.run( "rm -rf " + jsonfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --fields " + key +
      " --type json" +
      " --strict true" +
      " --sort '{ _id: 1 }'" +
      " --file " + jsonfile;
   testRunCommand( command );

   var jsonContent =
      "{ \"" + key + "\": { \"$numberLong\": \"" + longMin + "\" } }\n" +
      "{ \"" + key + "\": { \"$numberLong\": \"" + intMin + "\" } }\n" +
      "{ \"" + key + "\": { \"$numberLong\": \"" + intMax + "\" } }\n" +
      "{ \"" + key + "\": { \"$numberLong\": \"" + longMax + "\" } }\n";
   checkFileContent( jsonfile, jsonContent );

   command = installPath + "bin/sdbimprt" +
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

function testExprtStrict2 ()
{
   var jsonfile = tmpFileDir + "sdbexprt12236.json";
   cmd.run( "rm -rf " + jsonfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --type json" +
      " --fields " + key +
      " --strict false" +
      " --sort '{ _id: 1 }'" +
      " --file " + jsonfile;
   testRunCommand( command );

   var jsonContent =
      "{ \"" + key + "\": { \"$numberLong\": \"" + longMin + "\" } }\n" +
      "{ \"" + key + "\": " + intMin + " }\n" +
      "{ \"" + key + "\": " + intMax + " }\n" +
      "{ \"" + key + "\": { \"$numberLong\": \"" + longMax + "\" } }\n";
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