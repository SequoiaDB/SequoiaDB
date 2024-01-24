/*******************************************************************
* @Description : test export with --fields same cs multi cl
*                seqDB-13530:导出多个集合的多个字段
*                          （批量测试，如2048个）      
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME + "_13530";
var clnum = 100;
var clnames = [];
var doc = { a: 1 };
var csvContent = "a\n1\n";

main( test );

function test ()
{
   commDropCS( db, csname );
   commCreateCS( db, csname );

   for( var i = 0; i < clnum; i++ )
   {
      var clname = COMMCLNAME + "_sdbexprt13530_" + i;
      var cl = commCreateCL( db, csname, clname );
      cl.insert( doc );
      clnames.push( clname );
   }

   testExprtCsv();

   commDropCS( db, csname );
}

function testExprtCsv ()
{
   var csvDir = tmpFileDir + "13530/";
   cmd.run( "mkdir -p " + csvDir );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --cscl " + csname +
      " --dir " + csvDir +
      " --type csv";
   for( var i = 0; i < clnames.length; i++ )
   {
      command += " --fields " + csname + "." + clnames[i] + ":a";
   }
   testRunCommand( command );

   for( var i = 0; i < clnames.length; i++ )
   {
      var filename = csvDir + csname + "." + clnames[i] + ".csv";
      checkFileContent( filename, csvContent );
   }

   cmd.run( "rm -rf " + csvDir );
}
