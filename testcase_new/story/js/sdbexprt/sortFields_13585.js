/*******************************************************************
* @Description : test export with --sort
*                seqDB-13585:--sort指定对多个字段排序
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13585";

main( test );

function test ()
{
   var docs = [{ a: 1, b: 1 }, { a: 1, b: 2 },
   { a: 3 }, { a: 2 }, { a: 4 }];
   var cl = commCreateCL( db, csname, clname );
   cl.insert( docs );

   testExprtSort1();    // test sort with { a: 1, b: 1 }
   testExprtSort2();    // test sort with { a: -1, b: 1 }
   testExprtSort3();    // test sort with { a: 1, b: -1 }
   testExprtSort4();    // test sort with { a: -1, b: -1 }

   commDropCL( db, csname, clname );
}

function testExprtSort1 ()
{
   var csvfile = tmpFileDir + "sdbexprt13585.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --sort '{ a: 1, b: 1 }'" +
      " --fields a,b" +
      " --type csv";
   testRunCommand( command );

   var content = "a,b\n1,1\n1,2\n2,\n3,\n4,\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtSort2 ()
{
   var csvfile = tmpFileDir + "sdbexprt13585.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --sort '{ a: -1, b: 1 }'" +
      " --fields a,b" +
      " --type csv";
   testRunCommand( command );

   var content = "a,b\n4,\n3,\n2,\n1,1\n1,2\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtSort3 ()
{
   var csvfile = tmpFileDir + "sdbexprt13585.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --sort '{ a: 1, b: -1 }'" +
      " --fields a,b" +
      " --type csv";
   testRunCommand( command );

   var content = "a,b\n1,2\n1,1\n2,\n3,\n4,\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtSort4 ()
{
   var csvfile = tmpFileDir + "sdbexprt13585.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --sort '{ a: -1, b: -1 }'" +
      " --fields a,b" +
      " --type csv";
   testRunCommand( command );

   var content = "a,b\n4,\n3,\n2,\n1,2\n1,1\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
}