/*******************************************************************
* @Description : test export with --file
*                seqDB-13571:--file指定的文件路径不存在
*                seqDB-13572:--file指定的文件路径无读写权限
*                seqDB-13573:--file指定的文件已存在（指定/不指定--replace）              
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13571";
var doc = { a: 1 };
var csvContent = "a\n1\n";

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   cl.insert( doc );

   testExprtNoPath();
   testExprtNoPerm();
   testExprtExisted1();  // test file existed with --replace
   testExprtExisted2();  // test file existed without --replace 

   commDropCL( db, csname, clname );
}

function testExprtNoPath ()
{
   var csvDir = tmpFileDir + "13571/";
   var csvfile = csvDir + "sdbexprt13571.csv";
   cmd.run( "rm -rf " + csvDir );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields a";
   testRunCommand( command, 8 );

   cmd.run( "rm -rf " + csvDir );
}

function testExprtNoPerm ()
{
   var user = getCurrentUser();
   if( user === "root" )
   {
      return;
   }

   var csvDir = tmpFileDir + "13572/";
   var csvfile = csvDir + "sdbexprt13571.csv";
   cmd.run( "mkdir -p " + csvDir );
   File.chmod( csvDir, 0000 );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields a";
   testRunCommand( command, 129 );

   cmd.run( "rm -rf " + csvDir );
}

function testExprtExisted1 ()
{
   var csvfile = tmpFileDir + "sdbexprt13573.csv";
   cmd.run( "rm -rf " + csvfile );
   var file = new File( csvfile );
   file.write( "abcde" );
   file.close();

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields a" +
      " --replace";
   testRunCommand( command );

   checkFileContent( csvfile, csvContent );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtExisted2 ()
{
   var csvfile = tmpFileDir + "sdbexprt13573.csv";
   cmd.run( "rm -rf " + csvfile );
   var file = new File( csvfile );
   file.write( "abcde" );
   file.close();

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields a";
   testRunCommand( command, 8 );

   cmd.run( "rm -rf " + csvfile );
}