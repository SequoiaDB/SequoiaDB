/*******************************************************************
* @Description : test export with --cscl multi cs
*                seqDB-13565:不指定-c、-l、-cscl，导出所有数据
*                seqDB-13566:同时指定-c、-l、-cscl              
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
      var csname = COMMCSNAME + "_sdbexprt13565_" + i;
      var clname = COMMCLNAME + "_sdbexprt13565_" + i;
      var cl = commCreateCL( db, csname, clname );
      cl.insert( doc );
      clnames.push( clname );
      csnames.push( csname );
   }

   testExprtNoCsCl();
   testExprtCsCl();

   for( var i = 0; i < clnum; i++ )
   {
      commDropCS( db, csnames[i] );
   }
}

function testExprtNoCsCl ()
{
   var csvDir = tmpFileDir + "13565/";
   cmd.run( "mkdir -p " + csvDir );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --dir " + csvDir +
      " --type csv" +
      " --force true";
   testRunCommand( command );

   for( var i = 0; i < clnum; i++ )
   {
      var filename = csvDir + csnames[i] + "." + clnames[i] + ".csv";
      checkFileContent( filename, csvContent );
   }

   cmd.run( "rm -rf " + csvDir );
}

function testExprtCsCl ()
{
   var csvfile = tmpFileDir + "sdbexprt13566.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csnames[0] +
      " -l " + clnames[0] +
      " --cscl " + csnames[0] + "." + clnames[0] +
      " --file " + csvfile +
      " --type csv" +
      " --force true";
   testRunCommand( command, 127 );

   cmd.run( "rm -rf " + csvfile );
}