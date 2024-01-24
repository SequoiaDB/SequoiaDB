/*******************************************************************
* @Description : test export with --file --replace
*                seqDB-13989:--file指定的文件已存在，导出到配置文件
*                           （指定/不指定--replace）              
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13989";
var doc = { a: 1 };
var csvContent = "a\n1\n";

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   cl.insert( doc );

   testExprtReplace1();  // test file existed with --replace
   testExprtReplace2();  // test file existed without --replace 

   commDropCL( db, csname, clname );
}

function testExprtReplace1 ()
{
   var csvfile = tmpFileDir + "sdbexprt13989.csv";
   cmd.run( "rm -rf " + csvfile );
   var file = new File( csvfile );
   file.write( "abcde" );
   file.close();

   var confFile = tmpFileDir + "sdbexprt13989.conf";
   cmd.run( "rm -rf " + confFile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --genconf " + confFile +
      " --type csv" +
      " --fields a" +
      " --replace";
   testRunCommand( command );

   command = installPath + "bin/sdbexprt" +
      " --conf " + confFile +
      " --file " + csvfile;
   testRunCommand( command );

   checkFileContent( csvfile, csvContent );

   cmd.run( "rm -rf " + csvfile );
   cmd.run( "rm -rf " + confFile );
}

function testExprtReplace2 ()
{
   var csvfile = tmpFileDir + "sdbexprt13989.csv";
   cmd.run( "rm -rf " + csvfile );
   var file = new File( csvfile );
   file.write( "abcde" );
   file.close();

   var confFile = tmpFileDir + "sdbexprt13989.conf";
   cmd.run( "rm -rf " + confFile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --genconf " + confFile +
      " --type csv" +
      " --fields a";
   testRunCommand( command );

   command = installPath + "bin/sdbexprt" +
      " --conf " + confFile +
      " --file " + csvfile;
   testRunCommand( command, 8 );

   cmd.run( "rm -rf " + csvfile );
   cmd.run( "rm -rf " + confFile );
}