/*******************************************************************
* @Description : test export with --force --withid
*                seqDB-13545:字段顺序错乱，强制导出到json
*                seqDB-13546:字段顺序错乱，强制导出到csv
*                seqDB-13547:字段顺序错乱，非强制导出               
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;

main( test );

function test ()
{
   testForceCsv1();   // test force csv with fields out of order
   testForceCsv2();   // test no force with fields out of order
   testForceJson();   // test force json with fields out of order
}

function testForceCsv1 ()
{
   var clname = COMMCLNAME + "_sdbexprt13546";
   var clname1 = COMMCLNAME + "_sdbimprt13546";
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );
   cl.insert( { _id: 1, a: 1, b: 2, c: 3 } );
   cl.insert( { _id: 2, c: 1, b: 2, a: 3 } );

   var csvfile = tmpFileDir + "sdbexprt13546.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --withid true" +
      " --sort '{ _id: 1 }'" +
      " --force true";
   testRunCommand( command );

   var content = "_id,a,b,c\n1,1,2,3\n2,3,2,1\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --file " + csvfile +
      " --type csv " +
      " --fields='_id int,a int,b int,c int'" +
      " --headerline true" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   var expRecs = ["{\"_id\":1,\"a\":1,\"b\":2,\"c\":3}",
      "{\"_id\":2,\"a\":3,\"b\":2,\"c\":1}"];
   var cursor = cl1.find();
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );
   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}

function testForceCsv2 ()
{
   var clname = COMMCLNAME + "_sdbexprt13547";
   var clname1 = COMMCLNAME + "_sdbimprt13547";
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );
   cl.insert( { a: 1, b: 2, c: 3 } );
   cl.insert( { c: 1, b: 2, a: 3 } );

   var csvfile = tmpFileDir + "sdbexprt13547.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields a,b,c" +
      " --sort '{ _id: 1 }'" +
      " --force false";
   testRunCommand( command );

   var content = "a,b,c\n1,2,3\n3,2,1\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --file " + csvfile +
      " --type csv " +
      " --fields='a int,b int,c int'" +
      " --headerline true" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   var expRecs = ["{\"a\":1,\"b\":2,\"c\":3}",
      "{\"a\":3,\"b\":2,\"c\":1}"];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + csvfile );
   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}

function testForceJson ()
{
   var clname = COMMCLNAME + "_sdbexprt13545";
   var clname1 = COMMCLNAME + "_sdbimprt13545";
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );
   cl.insert( { _id: 1, a: 1, b: 2, c: 3 } );
   cl.insert( { _id: 2, c: 1, b: 2, a: 3 } );

   var jsonfile = tmpFileDir + "sdbexprt13545.json";
   cmd.run( "rm -rf " + jsonfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + jsonfile +
      " --type json" +
      " --withid true" +
      " --sort '{ _id: 1 }'" +
      " --force true";
   testRunCommand( command );

   var content = "{ \"_id\": 1, \"a\": 1, \"b\": 2, \"c\": 3 }\n" +
      "{ \"_id\": 2, \"c\": 1, \"b\": 2, \"a\": 3 }\n";
   checkFileContent( jsonfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --file " + jsonfile +
      " --type json" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   var expRecs = ["{\"_id\":1,\"a\":1,\"b\":2,\"c\":3}",
      "{\"_id\":2,\"c\":1,\"b\":2,\"a\":3}"];
   var cursor = cl1.find();
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cmd.run( "rm -rf " + jsonfile );
   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}