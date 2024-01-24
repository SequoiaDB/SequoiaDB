/*******************************************************************
* @Description : test export with string data
*                seqDB-13509:集合中已存在所有数据类型的数据以及边界值，
*                            导出所有数据                 
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13509_string";
var clname1 = COMMCLNAME + "_sdbimprt13509_string";
var key = "string";
var longStr = makeString( 15 * 1024 * 1024, 'x' );
var docs = [{ "string": "" },
{ "string": "~!@$%^&*()中" },
{ "string": "\"abc\"def" },
{ "string": longStr }];
var csvContent = key + "\n" +
   "\"\"\n" +
   "\"~!@$%^&*()中\"\n" +
   "\"\"\"abc\"\"def\"\n" +
   "\"" + longStr + "\"\n";
var jsonContent = "{ \"" + key + "\": \"\" }\n" +
   "{ \"" + key + "\": \"~!@$%^&*()中\" }\n" +
   "{ \"" + key + "\": \"\\\"abc\\\"def\" }\n" +
   "{ \"" + key + "\": \"" + longStr + "\" }\n";

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );

   cl.insert( docs );

   testExprtImprtCsv();
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   checkDocs( cursor, docs );
   cl1.truncate();

   testExprtImprtJson();
   cursor = cl1.find( {}, { _id: { $include: 0 } } );
   checkDocs( cursor, docs );

   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}

function testExprtImprtCsv ()
{
   var csvfile = tmpFileDir + "sdbexprt13509_string.csv";
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
      " --fields='" + key + " string'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtImprtJson ()
{
   var jsonfile = tmpFileDir + "sdbexprt13509_string.json";
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

function checkDocs ( cursor, docs )
{
   var idx = 0;
   var info;
   while( info = cursor.next() )
   {
      var obj = info.toObj();
      var expVal = docs[idx][key];
      var actVal = obj[key];
      if( actVal !== expVal )
      {
         throw new Error( "checkDocs fail,check doc" +
            expVal.slice( 0, 100 ) + actVal.slice( 0, 100 ) );
      }
      idx++;
   }
   assert.equal( idx, docs.length );
}