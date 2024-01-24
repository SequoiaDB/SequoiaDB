/*******************************************************************
* @Description : test export with special decimal data
*                seqDB-14002:导出、导入特殊decimal值
*                           （导入指定、不指定类型）
*                seqDB-14003:导出、导入特殊decimal值
*                           （导入指定、不指定精度）                 
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt14002_decimal";
var clname1 = COMMCLNAME + "_sdbimprt14002_decimal";
var key = "decimal";
var docs = [{ "decimal": { $decimal: "MAX" } },
{ "decimal": { $decimal: "MIN" } },
{ "decimal": { $decimal: "NaN" } },
{ "decimal": { $decimal: "-INF" } },
{ "decimal": { $decimal: "INF" } }];
var csvContent = key + "\n" +
   "MAX\nMIN\nNaN\nMIN\nMAX\n";
var jsonContent =
   "{ \"" + key + "\": { \"$decimal\": \"MAX\" } }\n" +
   "{ \"" + key + "\": { \"$decimal\": \"MIN\" } }\n" +
   "{ \"" + key + "\": { \"$decimal\": \"NaN\" } }\n" +
   "{ \"" + key + "\": { \"$decimal\": \"MIN\" } }\n" +
   "{ \"" + key + "\": { \"$decimal\": \"MAX\" } }\n";
// import without type
var csvRecs1 = [
   "{\"" + key + "\":\"MAX\"}",
   "{\"" + key + "\":\"MIN\"}",
   "{\"" + key + "\":NaN}",
   "{\"" + key + "\":\"MIN\"}",
   "{\"" + key + "\":\"MAX\"}"
]
// import with type
var csvRecs2 = [
   "{\"" + key + "\":{\"$decimal\":\"MAX\"}}",
   "{\"" + key + "\":{\"$decimal\":\"MIN\"}}",
   "{\"" + key + "\":{\"$decimal\":\"NaN\"}}",
   "{\"" + key + "\":{\"$decimal\":\"MIN\"}}",
   "{\"" + key + "\":{\"$decimal\":\"MAX\"}}"
];
var jsonRecs = csvRecs2;

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );

   cl.insert( docs );

   testExprtImprtCsv1();  // test import withou type
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   var recs = getRecords( cursor );
   checkRecords( csvRecs1, recs );
   cl1.truncate();

   testExprtImprtCsv2(); // test import with type without precison
   cursor = cl1.find( {}, { _id: { $include: 0 } } );
   recs = getRecords( cursor );
   checkRecords( csvRecs2, recs );
   cl1.truncate();

   testExprtImprtCsv3(); // test import with type with precison
   cursor = cl1.find( {}, { _id: { $include: 0 } } );
   recs = getRecords( cursor );
   checkRecords( csvRecs2, recs );
   cl1.truncate();

   testExprtImprtJson();
   cursor = cl1.find( {}, { _id: { $include: 0 } } );
   recs = getRecords( cursor );
   checkRecords( jsonRecs, recs );

   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}

function testExprtImprtCsv1 ()
{
   var csvfile = tmpFileDir + "sdbexprt14002_decimal.csv";
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
      " --parsers 1 -j 1";
   testRunCommand( command );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtImprtCsv2 ()
{
   var csvfile = tmpFileDir + "sdbexprt14002_decimal.csv";
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
      " --field '" + key + " decimal'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtImprtCsv3 ()
{
   var csvfile = tmpFileDir + "sdbexprt14002_decimal.csv";
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
      " --field '" + key + " decimal(1000,100)'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtImprtJson ()
{
   var jsonfile = tmpFileDir + "sdbexprt14002_decimal.json";
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
