/*******************************************************************
* @Description : test export with --fields same cs multi cl
*                seqDB-13528:导出相同CS下多个集合的多个字段      
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME + "_sdbexprt13528";
var clnum = 5;
var clnames = [];
var doc = { a: 1 };
var csvContent = "a\n1\n";

main( test );

function test ()
{
   for( var i = 0; i < clnum; i++ )
   {
      var clname = COMMCLNAME + "_sdbexprt13528_" + i;
      var cl = commCreateCL( db, csname, clname );
      cl.insert( doc );
      clnames.push( clname );
   }

   testExprtCsv();

   for( var i = 0; i < clnames.length; i++ )
   {
      commDropCS( db, csname );
   }
}

function testExprtCsv ()
{
   var csvDir = tmpFileDir + "13528/";
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
