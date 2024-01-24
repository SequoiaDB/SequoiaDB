/*******************************************************************
* @Description : test export with subobj data
*                seqDB-13509:集合中已存在所有数据类型的数据以及边界值，
*                            导出所有数据
*                seqDB-7032:sdbexprt将object类型的记录导成csv格式               
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13509_subobj";
var clname1 = COMMCLNAME + "_sdbimprt13509_subobj";
var key = "obj";
var docs = [{ "obj": { "sub1": { "sub2": { "sub3": { "sub4": { "sub5": { "sub6": { "sub7": { "sub8": { "sub9": { "sub10": { "sub11": { "sub12": { "sub13": { "sub14": { "sub15": { "sub16": { "sub17": { "sub18": { "sub19": { "sub20": { "sub21": { "sub22": { "sub23": { "sub24": { "sub25": { "sub26": { "sub27": { "sub28": { "sub29": { "sub30": { "sub31": 1 } } } } } } } } } } } } } } } } } } } } } } } } } } } } } } } }];
var csvContent =
   key + "\n" +
   "\"{ \"\"sub1\"\": { \"\"sub2\"\": { \"\"sub3\"\": { \"\"sub4\"\": { \"\"sub5\"\": { \"\"sub6\"\": { \"\"sub7\"\": { \"\"sub8\"\": { \"\"sub9\"\": { \"\"sub10\"\": { \"\"sub11\"\": { \"\"sub12\"\": { \"\"sub13\"\": { \"\"sub14\"\": { \"\"sub15\"\": { \"\"sub16\"\": { \"\"sub17\"\": { \"\"sub18\"\": { \"\"sub19\"\": { \"\"sub20\"\": { \"\"sub21\"\": { \"\"sub22\"\": { \"\"sub23\"\": { \"\"sub24\"\": { \"\"sub25\"\": { \"\"sub26\"\": { \"\"sub27\"\": { \"\"sub28\"\": { \"\"sub29\"\": { \"\"sub30\"\": { \"\"sub31\"\": 1 } } } } } } } } } } } } } } } } } } } } } } } } } } } } } } }\"\n";
var jsonContent =
   "{ \"obj\": { \"sub1\": { \"sub2\": { \"sub3\": { \"sub4\": { \"sub5\": { \"sub6\": { \"sub7\": { \"sub8\": { \"sub9\": { \"sub10\": { \"sub11\": { \"sub12\": { \"sub13\": { \"sub14\": { \"sub15\": { \"sub16\": { \"sub17\": { \"sub18\": { \"sub19\": { \"sub20\": { \"sub21\": { \"sub22\": { \"sub23\": { \"sub24\": { \"sub25\": { \"sub26\": { \"sub27\": { \"sub28\": { \"sub29\": { \"sub30\": { \"sub31\": 1 } } } } } } } } } } } } } } } } } } } } } } } } } } } } } } } }\n";
var csvRecs = [
   "{\"obj\":\"{ \\\"sub1\\\": { \\\"sub2\\\": { \\\"sub3\\\": { \\\"sub4\\\": { \\\"sub5\\\": { \\\"sub6\\\": { \\\"sub7\\\": { \\\"sub8\\\": { \\\"sub9\\\": { \\\"sub10\\\": { \\\"sub11\\\": { \\\"sub12\\\": { \\\"sub13\\\": { \\\"sub14\\\": { \\\"sub15\\\": { \\\"sub16\\\": { \\\"sub17\\\": { \\\"sub18\\\": { \\\"sub19\\\": { \\\"sub20\\\": { \\\"sub21\\\": { \\\"sub22\\\": { \\\"sub23\\\": { \\\"sub24\\\": { \\\"sub25\\\": { \\\"sub26\\\": { \\\"sub27\\\": { \\\"sub28\\\": { \\\"sub29\\\": { \\\"sub30\\\": { \\\"sub31\\\": 1 } } } } } } } } } } } } } } } } } } } } } } } } } } } } } } }\"}"
];
var jsonRecs = [
   "{\"obj\":{\"sub1\":{\"sub2\":{\"sub3\":{\"sub4\":{\"sub5\":{\"sub6\":{\"sub7\":{\"sub8\":{\"sub9\":{\"sub10\":{\"sub11\":{\"sub12\":{\"sub13\":{\"sub14\":{\"sub15\":{\"sub16\":{\"sub17\":{\"sub18\":{\"sub19\":{\"sub20\":{\"sub21\":{\"sub22\":{\"sub23\":{\"sub24\":{\"sub25\":{\"sub26\":{\"sub27\":{\"sub28\":{\"sub29\":{\"sub30\":{\"sub31\":1}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}"
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
   var csvfile = tmpFileDir + "sdbexprt13509_subobj.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --fields " + key +
      " --type csv" +
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

function testExprtImprtJson ()
{
   var jsonfile = tmpFileDir + "sdbexprt13509_subobj.json";
   cmd.run( "rm -rf " + jsonfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --type json" +
      " --fields " + key +
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