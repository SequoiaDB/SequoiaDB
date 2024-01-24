/*******************************************************************
* @Description : test export with --skip
*                seqDB-13587:--skip取值为0、负数
*                seqDB-13588:--skip指定跳过N条记录（1<=N<=总记录数）
*                seqDB-13589:--skip指定跳过的记录数大于总记录数
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13587";

main( test );

function test ()
{
   var docs = [{ a: 1 }, { a: 3 }, { a: 2 }, { a: 4 }];
   var cl = commCreateCL( db, csname, clname );
   cl.insert( docs );

   testExprtSkip1();    // test skip 0
   testExprtSkip2();    // test skip 2
   testExprtSkip3();    // test skip 5
   testExprtSkip4();    // test skip -1

   commDropCL( db, csname, clname );
}

function testExprtSkip1 ()
{
   var csvfile = tmpFileDir + "sdbexprt13587.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --skip 0" +
      " --fields a" +
      " --sort '{ _id: 1 }'" +
      " --type csv";
   testRunCommand( command );

   var content = "a\n1\n3\n2\n4\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtSkip2 ()
{
   var csvfile = tmpFileDir + "sdbexprt13588.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --skip 2" +
      " --fields a" +
      " --sort '{ _id: 1 }'" +
      " --type csv";
   testRunCommand( command );

   var content = "a\n2\n4\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtSkip3 ()
{
   var csvfile = tmpFileDir + "sdbexprt13589.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --skip 5" +
      " --fields a" +
      " --type csv";
   testRunCommand( command );

   var content = "a\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtSkip4 ()
{
   var csvfile = tmpFileDir + "sdbexprt13587.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --skip -1" +
      " --fields a" +
      " --sort '{ _id: 1 }'" +
      " --type csv";
   testRunCommand( command );

   var content = "a\n1\n3\n2\n4\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + csvfile );
}