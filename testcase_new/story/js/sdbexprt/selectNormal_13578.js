/*******************************************************************
* @Description : test export with --select
*                seqDB-13578:--select指定正确的查询条件
*                选择符：$include
*                        $default
*                        $elemMatch $elemMatchOne
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13578";
var clname1 = COMMCLNAME + "_sdbimprt13578";

main( test );

function test ()
{
   testExprtSelect1();  // no use select symbol
   testExprtSelect2();  // use $include
   testExprtSelect3();  // use $default
   testExprtSelect4();  // use $elemMatch
}

function testExprtSelect1 ()
{
   var docs = [{ a: 1 }, { a: 2 }, { a: 3 }, { a: 4 }];
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13578.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --select '{ a: \"\" }'" +
      " --sort '{ _id: 1 }'" +
      " --type csv";
   testRunCommand( command );

   var expContent = "a\n1\n2\n3\n4\n";
   checkFileContent( csvfile, expContent );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --file " + csvfile +
      " --type csv" +
      " --headerline true" +
      " --fields='a int'";
   testRunCommand( command );

   var expRecs = ["{\"a\":1}", "{\"a\":2}",
      "{\"a\":3}", "{\"a\":4}"];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );
   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}

function testExprtSelect2 ()
{
   var docs = [{ a: 1, b: 1 }, { a: 2, b: 2 },
   { a: 3, b: 3 }, { a: 4, b: 4 }];
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13578.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --select '{ a: { \\$include: 1 } }'" +
      " --sort '{ _id: 1 }'" +
      " --type csv";
   testRunCommand( command );

   var expContent = "a\n1\n2\n3\n4\n";
   checkFileContent( csvfile, expContent );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --file " + csvfile +
      " --type csv" +
      " --headerline true" +
      " --fields='a int'";
   testRunCommand( command );

   var expRecs = ["{\"a\":1}", "{\"a\":2}",
      "{\"a\":3}", "{\"a\":4}"];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );
   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}

function testExprtSelect3 ()
{
   var docs = [{ a: 1, b: 1 }, { a: 2, b: 2 },
   { a: 3 }, { a: 4 }];
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13578.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --select '{ a: { \\$include: 1 }, b: { \\$default: 3 } }'" +
      " --sort '{ _id: 1 }'" +
      " --type csv";
   testRunCommand( command );

   var expContent = "a,b\n1,1\n2,2\n3,3\n4,3\n";
   checkFileContent( csvfile, expContent );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --file " + csvfile +
      " --type csv" +
      " --headerline true" +
      " --fields='a int,b int'";
   testRunCommand( command );

   var expRecs = ["{\"a\":1,\"b\":1}", "{\"a\":2,\"b\":2}",
      "{\"a\":3,\"b\":3}", "{\"a\":4,\"b\":3}"];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );
   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}

function testExprtSelect4 ()
{
   var docs = [{
      class: "16110902",
      "students": [{ "name": "lxw", "age": 18 },
      { "name": "lfm", "age": 19 },
      { "name": "dyy", "age": 18 }]
   }];
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );
   cl.insert( docs );

   var csvfile = tmpFileDir + "sdbexprt13578.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --select " +
      "'{ class: { \\$include: 1 }, students: { \\$elemMatch: { age: 18 } } }'" +
      " --sort '{ _id: 1 }'" +
      " --type csv";
   testRunCommand( command );

   var expContent = "class,students\n" +
      "\"16110902\",\"[ { \"\"name\"\": \"\"lxw\"\", \"\"age\"\": 18 }," +
      " { \"\"name\"\": \"\"dyy\"\", \"\"age\"\": 18 } ]\"\n";
   checkFileContent( csvfile, expContent );

   // import csv
   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --file " + csvfile +
      " --type csv" +
      " --headerline true" +
      " --fields='class string,students string'";
   testRunCommand( command );

   var expRecs = [
      "{\"class\":\"16110902\",\"students\":\"[" +
      " { \\\"name\\\": \\\"lxw\\\", \\\"age\\\": 18 }," +
      " { \\\"name\\\": \\\"dyy\\\", \\\"age\\\": 18 } ]\"}"];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );
   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}
