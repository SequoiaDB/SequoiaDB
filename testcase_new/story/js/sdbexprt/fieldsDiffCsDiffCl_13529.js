/*******************************************************************
* @Description : test export with --fields multi cs multi cl
*                seqDB-13529:导出不同CS下多个集合的多个字段               
* @author      : Liang XueWang 
*
*******************************************************************/
var num = 5;
var csnames = [];
var clnames = [];
var doc = { a: 1 };
var csvContent = "a\n1\n";

main( test );

function test ()
{
   for( var i = 0; i < num; i++ )
   {
      var csname = COMMCSNAME + "_sdbexprt13529_" + i;
      var clname = COMMCLNAME + "_sdbexprt13529_" + i;
      var cl = commCreateCL( db, csname, clname );
      cl.insert( doc );
      csnames.push( csname );
      clnames.push( clname );
   }

   testExprtCsv();

   for( var i = 0; i < num; i++ )
   {
      commDropCS( db, csnames[i] );
   }
}

function testExprtCsv ()
{
   var csvDir = tmpFileDir + "13529/";
   cmd.run( "mkdir -p " + csvDir );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --dir " + csvDir +
      " --type csv";
   for( var i = 0; i < num; i++ )
   {
      command += " --fields " + csnames[i] + "." + clnames[i] + ":a";
   }
   command += " --cscl ";
   for( var i = 0; i < num; i++ )
   {
      command += csnames[i];
      if( i !== num - 1 ) command += ",";
   }
   testRunCommand( command );

   for( var i = 0; i < num; i++ )
   {
      var filename = csvDir + csnames[i] + "." + clnames[i] + ".csv";
      checkFileContent( filename, csvContent );
   }

   cmd.run( "rm -rf " + csvDir );
}