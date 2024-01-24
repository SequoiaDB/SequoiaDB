/*******************************************************************
* @Description : test export with illegal --filelimit
*                seqDB-13521:指定文件大小上限为0K
*                seqDB-13523:指定文件大小上限为xK，x为负数               
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13521";
var filelimits = ["0K", "-3K"];
var filelimit;
var kb = 1024;

main( test );

function test ()
{
   for( var i = 0; i < filelimits.length; i++ )
   {
      filelimit = filelimits[i];
      testFileLimit();
   }
}

function testFileLimit ()
{
   var cl = commCreateCL( db, csname, clname );
   cl.insert( { a: 1 } );

   testExprtCsv();

   commDropCL( db, csname, clname );
}

function testExprtCsv ()
{
   var csvfile = tmpFileDir + "sdbexprt13521.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --filelimit " + filelimit +
      " --type csv" +
      " --fields a";
   testRunCommand( command, 127 );

   cmd.run( "rm -rf " + csvfile );
}