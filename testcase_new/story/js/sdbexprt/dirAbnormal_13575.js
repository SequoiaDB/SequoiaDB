/*******************************************************************
* @Description : test export with --dir
*                seqDB-13575:--dir目录无读写权限
*                seqDB-13576:--dir目录不存在
*                seqDB-13577:--dir目录下cs.cl.csv文件已存在              
* @author      : Liang XueWang 
*
*******************************************************************/
var clnum = 5;
var clnames = [];
var csnames = [];
var doc = { a: 1 };
var csvContent = "a\n1\n";

main( test );

function test ()
{
   for( var i = 0; i < clnum; i++ )
   {
      var csname = COMMCSNAME + "_sdbexprt13575_" + i;
      var clname = COMMCLNAME + "_sdbexprt13575_" + i;
      var cl = commCreateCL( db, csname, clname );
      cl.insert( doc );
      clnames.push( clname );
      csnames.push( csname );
   }

   testExprtNoPerm();
   testExprtNoDir();
   testExprtExisted1();   // test export existed with --replace
   testExprtExisted2();   // test export existed without --replace

   for( var i = 0; i < clnum; i++ )
   {
      commDropCS( db, csnames[i] );
   }
}

function testExprtNoPerm ()
{
   var user = getCurrentUser();
   if( user === "root" )
   {
      return;
   }

   var csvDir = tmpFileDir + "13575/";
   cmd.run( "mkdir -p " + csvDir );
   File.chmod( csvDir, 0000 );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --dir " + csvDir +
      " --type csv" +
      " --force true";
   command += " --cscl ";
   for( var i = 0; i < clnum; i++ )
   {
      command += csnames[i];
      if( i !== clnum - 1 )
         command += ",";
   }

   testRunCommand( command, 127 );

   cmd.run( "rm -rf " + csvDir );
}

function testExprtNoDir ()
{
   var csvDir = tmpFileDir + "13576/";
   cmd.run( "rm -rf " + csvDir );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --dir " + csvDir +
      " --type csv" +
      " --force true";
   command += " --cscl ";
   for( var i = 0; i < clnum; i++ )
   {
      command += csnames[i];
      if( i !== clnum - 1 )
         command += ",";
   }

   testRunCommand( command, 127 );

   cmd.run( "rm -rf " + csvDir );
}

function testExprtExisted1 ()
{
   var csvDir = tmpFileDir + "13577/";
   cmd.run( "mkdir -p " + csvDir );
   var csvfile = csvDir + csnames[0] + "." + clnames[0] + ".csv";
   var file = new File( csvfile );
   file.write( "abcdefghijk" );
   file.close();

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --dir " + csvDir +
      " --type csv" +
      " --replace" +
      " --force true";
   command += " --cscl ";
   for( var i = 0; i < clnum; i++ )
   {
      command += csnames[i];
      if( i !== clnum - 1 )
         command += ",";
   }

   testRunCommand( command );

   checkFileContent( csvfile, csvContent );

   cmd.run( "rm -rf " + csvDir );
}

function testExprtExisted2 ()
{
   var csvDir = tmpFileDir + "13577/";
   cmd.run( "mkdir -p " + csvDir );
   var csvfile = csvDir + csnames[0] + "." + clnames[0] + ".csv";
   var file = new File( csvfile );
   file.write( "abcdefghijk" );
   file.close();

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --dir " + csvDir +
      " --type csv" +
      " --force true";
   command += " --cscl ";
   for( var i = 0; i < clnum; i++ )
   {
      command += csnames[i];
      if( i !== clnum - 1 )
         command += ",";
   }

   testRunCommand( command, 8 );

   cmd.run( "rm -rf " + csvDir );
}