/*******************************************************************
* @Description : test export with --filter
*                seqDB-13581:--filter指定正确的查询条件
*                匹配符：$gt $gte $lt $lte $ne $et 
*                        $mod 
*                        $in $nin
*                        $isnull
*                        $all 
*                        $and $not $or $type（变成函数操作） 
*                        $exists
*                        $elemMatch 
*                        $1 $size（变成函数操作） 
*                        $regex 
*                        $field 
*                        $expand    
*                        $returnMatch
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13581";
var clname1 = COMMCLNAME + "_sdbimprt13581";

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );
   testExprtFilter1( cl, cl1 );  // test filter with $gte
   testExprtFilter2( cl, cl1 );  // test filter with $mod 
   testExprtFilter3( cl, cl1 );  // test filter with $in
   testExprtFilter4( cl, cl1 );  // test filter with $isnull
   testExprtFilter5( cl, cl1 );  // test filter with $all
   testExprtFilter6( cl, cl1 );  // test filter with $and
   testExprtFilter7( cl, cl1 );  // test filter with $exists
   testExprtFilter8( cl, cl1 );  // test filter with $elemMatch
   testExprtFilter9( cl, cl1 );  // test filter with $1
   testExprtFilter10( cl, cl1 );  // test filter with $regex
   testExprtFilter11( cl, cl1 );  // test filter with $field
   testExprtFilter12( cl, cl1 );  // test filter with $expand
   testExprtFilter13( cl, cl1 );  // test filter with $returnMatch
   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}

function testExprtFilter1 ( cl, cl1 )
{
   var docs = [{ a: 1 }, { a: 2 }, { a: 3 }, { a: 4 }];
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13581.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --filter '{ a: { \\$gte: 2 } }'" +
      " --type csv" +
      " --sort '{ _id: 1 }'" +
      " --fields a";
   testRunCommand( command );

   var content = "a\n2\n3\n4\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --type csv" +
      " --file " + csvfile +
      " --headerline true" +
      " --fields='a int'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   var expRecs = ["{\"a\":2}", "{\"a\":3}", "{\"a\":4}"];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );

   cl.truncate();
   cl1.truncate();
}

function testExprtFilter2 ( cl, cl1 )
{
   var docs = [{ a: 1 }, { a: 2 }, { a: 3 }, { a: 4 }];
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13581.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --filter '{ a: { \\$mod: [ 2, 1 ] } }'" +
      " --type csv" +
      " --sort '{ _id: 1 }'" +
      " --fields a";
   testRunCommand( command );

   var content = "a\n1\n3\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --type csv" +
      " --file " + csvfile +
      " --headerline true" +
      " --fields='a int'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   var expRecs = ["{\"a\":1}", "{\"a\":3}"];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );

   cl.truncate();
   cl1.truncate();
}

function testExprtFilter3 ( cl, cl1 )
{
   var docs = [{ a: 1 }, { a: 2 }, { a: 3 }, { a: 4 }];
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13581.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --filter '{ a: { \\$in: [ 1, 3, 4 ] } }'" +
      " --type csv" +
      " --sort '{ _id: 1 }'" +
      " --fields a";
   testRunCommand( command );

   var content = "a\n1\n3\n4\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --type csv" +
      " --file " + csvfile +
      " --headerline true" +
      " --fields='a int'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   var expRecs = ["{\"a\":1}", "{\"a\":3}", "{\"a\":4}"];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );

   cl.truncate();
   cl1.truncate();
}

function testExprtFilter4 ( cl, cl1 )
{
   var docs = [{ a: 1 }, { a: 2 }, { a: 3 }, { a: 4 }];
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13581.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --filter '{ a: { \\$isnull: 0 } }'" +
      " --type csv" +
      " --sort '{ _id: 1 }'" +
      " --fields a";
   testRunCommand( command );

   var content = "a\n1\n2\n3\n4\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --type csv" +
      " --file " + csvfile +
      " --headerline true" +
      " --fields='a int'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   var expRecs = ["{\"a\":1}", "{\"a\":2}", "{\"a\":3}", "{\"a\":4}"];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );

   cl.truncate();
   cl1.truncate();
}

function testExprtFilter5 ( cl, cl1 )
{
   var docs = [{ name: ["Tom", "Mike", "Jack"] },
   { name: ["Tom", "John"] }];
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13581.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --filter '{ name: { \\$all: [ \"Tom\", \"Mike\" ] } }'" +
      " --type csv" +
      " --fields name";
   testRunCommand( command );

   var content = "name\n" +
      "\"[ \"\"Tom\"\", \"\"Mike\"\", \"\"Jack\"\" ]\"\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --type csv" +
      " --file " + csvfile +
      " --headerline true" +
      " --fields='name string'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   var expRecs = ["{\"name\":\"[ \\\"Tom\\\", \\\"Mike\\\", \\\"Jack\\\" ]\"}"];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );

   cl.truncate();
   cl1.truncate();
}

function testExprtFilter6 ( cl, cl1 )
{
   var docs = [{ a: 1 }, { a: 2 }, { a: 3 }, { a: 4 }];
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13581.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --filter '{ \\$and: [ { a: { \\$gte: 2 } }, { a: { \\$lte: 3 } } ] }'" +
      " --type csv" +
      " --sort '{ _id: 1 }'" +
      " --fields a";
   testRunCommand( command );

   var content = "a\n2\n3\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --type csv" +
      " --file " + csvfile +
      " --headerline true" +
      " --fields='a int'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   var expRecs = ["{\"a\":2}", "{\"a\":3}"];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );

   cl.truncate();
   cl1.truncate();
}

function testExprtFilter7 ( cl, cl1 )
{
   var docs = [{ a: 1 }, { a: 2, b: 2 }, { a: 3 }, { a: 4, b: 4 }];
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13581.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --filter '{ b: { \\$exists: 1 } }'" +
      " --type csv" +
      " --sort '{ _id: 1 }'" +
      " --fields a,b";
   testRunCommand( command );

   var content = "a,b\n2,2\n4,4\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --type csv" +
      " --file " + csvfile +
      " --headerline true" +
      " --fields='a int,b int'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   var expRecs = ["{\"a\":2,\"b\":2}", "{\"a\":4,\"b\":4}"];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );

   cl.truncate();
   cl1.truncate();
}

function testExprtFilter8 ( cl, cl1 )
{
   var docs = [{ info: { name: "Jack", phone: "1234" } },
   { info: [{ name: "Jack", phone: "5678" }] }];
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13581.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --filter '{ info: { \\$elemMatch: { name: \"Jack\" } } }'" +
      " --type csv" +
      " --sort '{ _id: 1 }'" +
      " --fields info";
   testRunCommand( command );

   var content = "info\n" +
      "\"{ \"\"name\"\": \"\"Jack\"\", \"\"phone\"\": \"\"1234\"\" }\"\n" +
      "\"[ { \"\"name\"\": \"\"Jack\"\", \"\"phone\"\": \"\"5678\"\" } ]\"\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --type csv" +
      " --file " + csvfile +
      " --headerline true" +
      " --fields='info string'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   var expRecs = [
      "{\"info\":\"{ \\\"name\\\": \\\"Jack\\\", \\\"phone\\\": \\\"1234\\\" }\"}",
      "{\"info\":\"[ { \\\"name\\\": \\\"Jack\\\", \\\"phone\\\": \\\"5678\\\" } ]\"}"
   ];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );

   cl.truncate();
   cl1.truncate();
}

function testExprtFilter9 ( cl, cl1 )
{
   var docs = [{ a: [1, 2, 3, 4, 5] },
   { a: [1, 4, 5] }, { a: [1, 2, 4] }];
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13581.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --filter '{ a.\\$1: 5 }'" +
      " --type csv" +
      " --sort '{ _id: 1 }'" +
      " --fields a";
   testRunCommand( command );

   var content = "a\n" +
      "\"[ 1, 2, 3, 4, 5 ]\"\n" +
      "\"[ 1, 4, 5 ]\"\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --type csv" +
      " --file " + csvfile +
      " --headerline true" +
      " --fields='a string'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   var expRecs = [
      "{\"a\":\"[ 1, 2, 3, 4, 5 ]\"}",
      "{\"a\":\"[ 1, 4, 5 ]\"}"
   ];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );

   cl.truncate();
   cl1.truncate();
}

function testExprtFilter10 ( cl, cl1 )
{
   var docs = [{ a: "abandon" }, { a: "Alice" }, { a: "beyond" }];
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13581.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --filter '{ a: { \\$regex: \"^a\", \\$options: \"i\" } }'" +
      " --type csv" +
      " --sort '{ _id: 1 }'" +
      " --fields a";
   testRunCommand( command );

   var content = "a\n\"abandon\"\n\"Alice\"\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --type csv" +
      " --file " + csvfile +
      " --headerline true" +
      " --fields='a string'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   var expRecs = ["{\"a\":\"abandon\"}", "{\"a\":\"Alice\"}"];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );

   cl.truncate();
   cl1.truncate();
}

function testExprtFilter11 ( cl, cl1 )
{
   var docs = [{ a: 1, b: 1 }, { a: 2, b: 1 }, { a: 3, b: 3 },
   { a: 4, b: 3 }];
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13581.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --filter '{ a: { \\$field: \"b\" } }'" +
      " --type csv" +
      " --sort '{ _id: 1 }'" +
      " --fields a,b";
   testRunCommand( command );

   var content = "a,b\n1,1\n3,3\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --type csv" +
      " --file " + csvfile +
      " --headerline true" +
      " --fields='a int,b int'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   var expRecs = ["{\"a\":1,\"b\":1}", "{\"a\":3,\"b\":3}"];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );

   cl.truncate();
   cl1.truncate();
}

function testExprtFilter12 ( cl, cl1 )
{
   var docs = [{ a: [1, 2, 3] }];
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13581.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --filter '{ a: { \\$expand: 1 } }'" +
      " --type csv" +
      " --fields a";
   testRunCommand( command );

   var content = "a\n1\n2\n3\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --type csv" +
      " --file " + csvfile +
      " --headerline true" +
      " --fields='a int'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   var expRecs = ["{\"a\":1}", "{\"a\":2}", "{\"a\":3}"];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );

   cl.truncate();
   cl1.truncate();
}

function testExprtFilter13 ( cl, cl1 )
{
   var docs = [{ a: [1, 2, 4, 7, 9] }];
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13581.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --filter '{ a: { \\$returnMatch: 0, \\$in: [ 1, 4, 7 ] } }'" +
      " --type csv" +
      " --fields a";
   testRunCommand( command );

   var content = "a\n\"[ 1, 4, 7 ]\"\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --type csv" +
      " --file " + csvfile +
      " --headerline true" +
      " --fields='a string'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   var expRecs = ["{\"a\":\"[ 1, 4, 7 ]\"}"];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );

   cl.truncate();
   cl1.truncate();
}
