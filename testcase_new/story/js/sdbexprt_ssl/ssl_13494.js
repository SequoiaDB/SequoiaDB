/*******************************************************************
* @Description : test export with --ssl true
*                seqDB-13494:使用SSL连接               
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13494";
var doc = { a: 1 };
var csvContent = "a\n1\n";

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   cl.insert( doc );
   testExprtSSL();
   commDropCL( db, csname, clname );
}

function testExprtSSL ()
{
   var csvfile = tmpFileDir + "sdbexprt13494.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --ssl true" +
      " --file " + csvfile +
      " --type csv" +
      " --fields a";
   testRunCommand( command );

   checkFileContent( csvfile, csvContent );

   cmd.run( "rm -rf " + csvfile );
}