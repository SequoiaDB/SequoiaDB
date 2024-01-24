/*******************************************************************
* @Description : test export with illegal arguments
*                seqDB-13498:命令行多了支持的参数
*                seqDB-13499:命令行多了不支持的参数，如aaa
*                seqDB-13500:命令行缺少必填参数                 
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13498";
var doc = { a: 1 };
var csvContent = "a\n1\n";

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );

   cl.insert( doc );

   testExprtArg1();  // test export with extra supported arg
   testExprtArg2();  // test export with extra nonsupported arg
   testExprtArg3();  // test export without essential arg

   commDropCL( db, csname, clname );
}

function testExprtArg1 ()
{
   // export to csv with json option strict
   var csvfile = tmpFileDir + "sdbexprt13498.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --fields a" +
      " --type csv" +
      " --strict true" +
      " --file " + csvfile;
   testRunCommand( command );

   checkFileContent( csvfile, csvContent );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtArg2 ()
{
   // export to csv with nonsupport arg test
   var csvfile = tmpFileDir + "sdbexprt13499.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --type csv" +
      " --fields a" +
      " --test aaa" +
      " --file " + csvfile;
   testRunCommand( command, 127 );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtArg3 ()
{
   // export to csv without essentail arg fields
   var csvfile = tmpFileDir + "sdbexprt13500.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --type csv" +
      " --file " + csvfile;
   testRunCommand( command, 127 );

   cmd.run( "rm -rf " + csvfile );
}