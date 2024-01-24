/*******************************************************************
* @Description : test export with --limit
*                seqDB-13591:--limit -1导出所有记录
*                seqDB-13595:--limit 0导出记录
*                seqDB-13596:--limit > 总记录数  
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13591";

main( test );

function test ()
{
   var docs = [{ a: 1 }, { a: 3 }, { a: 2 }, { a: 4 }];
   var cl = commCreateCL( db, csname, clname );
   cl.insert( docs );

   testExprtLimit1();    // test limit -1
   testExprtLimit2();    // test limit 0
   testExprtLimit3();    // test limit 5

   commDropCL( db, csname, clname );
}

function testExprtLimit1 ()
{
   var csvfile = tmpFileDir + "sdbexprt13591.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --limit -1" +
      " --fields a" +
      " --sort '{ _id: 1 }'" +
      " --type csv";
   testRunCommand( command );

   var content = "a\n1\n3\n2\n4\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtLimit2 ()
{
   var csvfile = tmpFileDir + "sdbexprt13595.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --limit 0" +
      " --fields a" +
      " --type csv";
   testRunCommand( command );

   var content = "a\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtLimit3 ()
{
   var csvfile = tmpFileDir + "sdbexprt13596.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --limit 5" +
      " --fields a" +
      " --sort '{ _id: 1 }'" +
      " --type csv";
   testRunCommand( command );

   var content = "a\n1\n3\n2\n4\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
}