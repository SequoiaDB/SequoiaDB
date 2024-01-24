/*******************************************************************
* @Description : test export with --filter
*                seqDB-13582:--filter取值非法
*                seqDB-13583:--filter与--fields同时使用
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13582";
var docs = [{ a: 1 }, { a: 2 }, { a: 3 }, { a: 4 }];

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   cl.insert( docs );

   testExprtFilter1();  // use --filter '{ a: { \\$gt: 2 }'
   testExprtFilter2();  // use --filter '{ a: { \\$gtt: 2 } }'
   testExprtFilter3();  // use --filter with --fields

   commDropCL( db, csname, clname );
}

function testExprtFilter1 ()
{
   var csvfile = tmpFileDir + "sdbexprt13582.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --filter '{ a: { \\$gt: 2 }'" +
      " --type csv" +
      " --fields a";
   testRunCommand( command, 127 );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtFilter2 ()
{
   var csvfile = tmpFileDir + "sdbexprt13582.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --filter '{ a: { \\$gtt: 2 } }'" +
      " --type csv" +
      " --fields a";
   testRunCommand( command, 127 );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtFilter3 ()
{
   var csvfile = tmpFileDir + "sdbexprt13582.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --filter '{ a: { \\$gt: 2 } }'" +
      " --type csv" +
      " --sort '{ _id: 1 }'" +
      " --fields a";
   testRunCommand( command );

   var content = "a\n3\n4\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
}
