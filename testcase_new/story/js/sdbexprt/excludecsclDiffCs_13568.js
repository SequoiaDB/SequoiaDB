/*******************************************************************
* @Description : test export with --excludecscl
*                seqDB-13568:--excludecscl指定排除不同CS下的多个集合                
* @author      : Liang XueWang
* 
*******************************************************************/
var csnum = 2;
var clnum = 5;
var clnames = [];
var csnames = [];
var doc = { a: 1 };
var csvContent = "a\n1\n";

main( test );

function test ()
{
   for( var i = 0; i < csnum; i++ )
   {
      var csname = COMMCSNAME + "_sdbexprt13568_" + i;
      for( var j = 0; j < clnum; j++ )
      {
         var clname = COMMCLNAME + "_sdbexprt13568_" + j;
         var cl = commCreateCL( db, csname, clname );
         cl.insert( doc );
         if( i == 0 )
            clnames.push( clname );
      }
      csnames.push( csname );
   }

   testExcludeCsCl();

   for( var i = 0; i < csnum; i++ )
   {
      commDropCS( db, csnames[i] );
   }
}

function testExcludeCsCl ()
{
   var csvDir = tmpFileDir + "13568/";
   cmd.run( "mkdir -p " + csvDir );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --dir " + csvDir +
      " --type csv" +
      " --force true";
   command += " --cscl " + csnames[0] + "," + csnames[1];
   command += " --excludecscl " + csnames[0] + "." + clnames[0] + "," +
      csnames[0] + "." + clnames[1] + "," +
      csnames[1] + "." + clnames[0] + "," +
      csnames[1] + "." + clnames[1];
   testRunCommand( command );

   for( var i = 0; i < csnum; i++ )
   {
      for( var j = 0; j < clnum; j++ )
      {
         var filename = csvDir + csnames[i] + "." + clnames[j] + ".csv";
         if( ( i == 0 || i == 1 ) && ( j == 0 || j == 1 ) )
            checkFileExist( filename, false );
         else
            checkFileContent( filename, csvContent );
      }
   }

   cmd.run( "rm -rf " + csvDir );
}