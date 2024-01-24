/*******************************************************************
* @Description : test export with date data
*                seqDB-13509:集合中已存在所有数据类型的数据以及边界值，
*                            导出所有数据                 
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13509_date";
var clname1 = COMMCLNAME + "_sdbimprt13509_date";
var key = "date";
var docs = [{ "date": SdbDate( "1900-01-01" ) },
{ "date": SdbDate( "9999-12-31" ) },
{ "date": SdbDate( "0001-01-01T00:00:00.000Z" ) },
// { "date": SdbDate( "9999-12-31T23:59:59.999Z" ) },
{ "date": SdbDate( "0001-01-01T00:00:00.000+0800" ) },
{ "date": SdbDate( "9999-12-31T23:59:59.999+0800" ) },
{ "date": { $date: 9007199254740992 } },
{ "date": { $date: -9007199254740992 } }];
var csvContent = key + "\n" +
   "\"1900-01-01\"\n" +
   "\"9999-12-31\"\n" +
   "\"0001-01-01\"\n" +
   // "253402300799999\n" +
   "\"0001-01-01\"\n" +
   "\"9999-12-31\"\n" +
   "9007199254740992\n" +
   "-9007199254740992\n";
var jsonContent = "{ \"" + key + "\": { \"$date\": \"1900-01-01\" } }\n" +
   "{ \"" + key + "\": { \"$date\": \"9999-12-31\" } }\n" +
   "{ \"" + key + "\": { \"$date\": \"0001-01-01\" } }\n" +
   "{ \"" + key + "\": { \"$date\": \"0001-01-01\" } }\n" +
   "{ \"" + key + "\": { \"$date\": \"9999-12-31\" } }\n" +
   "{ \"" + key + "\": { \"$date\": 9007199254740992 } }\n" +
   "{ \"" + key + "\": { \"$date\": -9007199254740992 } }\n";
var csvRecs = ["{\"" + key + "\":{\"$date\":\"1900-01-01\"}}",
"{\"" + key + "\":{\"$date\":\"9999-12-31\"}}",
"{\"" + key + "\":{\"$date\":\"0001-01-01\"}}",
"{\"" + key + "\":{\"$date\":\"0001-01-01\"}}",
"{\"" + key + "\":{\"$date\":\"9999-12-31\"}}",
"{\"" + key + "\":{\"$date\":9007199254740992}}",
"{\"" + key + "\":{\"$date\":-9007199254740992}}"];
var jsonRecs = csvRecs;

main( test );

function test ()
{
   if ( commIsArmArchitecture() == false )
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
}

function testExprtImprtCsv ()
{
   var csvfile = tmpFileDir + "sdbexprt13509_date.csv";
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
      " --fields='" + key + " autodate'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtImprtJson ()
{
   var jsonfile = tmpFileDir + "sdbexprt13509_date.json";
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
