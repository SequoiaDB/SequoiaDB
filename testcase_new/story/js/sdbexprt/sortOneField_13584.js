/*******************************************************************
* @Description : test export with --sort
*                seqDB-13584:--sort指定对一个字段排序
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13584";

main( test );

function test ()
{
   var docs = [{ a: 1 }, { a: 3 }, { a: 2 }, { a: 4 }];
   var cl = commCreateCL( db, csname, clname );
   cl.insert( docs );

   testExprtSort1();    // test sort with { a: 1 }
   testExprtSort2();    // test sort with { a: -1 }

   commDropCL( db, csname, clname );
}

function testExprtSort1 ()
{
   var csvfile = tmpFileDir + "sdbexprt13584.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --sort '{ a: 1 }'" +
      " --fields a" +
      " --type csv";
   testRunCommand( command );

   var content = "a\n1\n2\n3\n4\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtSort2 ()
{
   var csvfile = tmpFileDir + "sdbexprt13584.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --sort '{ a: -1 }'" +
      " --fields a" +
      " --type csv";
   testRunCommand( command );

   var content = "a\n4\n3\n2\n1\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
}