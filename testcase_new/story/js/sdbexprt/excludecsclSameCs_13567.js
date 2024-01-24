/*******************************************************************
* @Description : test export with --excludecscl
*                seqDB-13567:--excludecscl指定排除相同CS下的多个集合            
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clnum = 5;
var clnames = [];
var doc = { a: 1 };
var csvContent = "a\n1\n";

main( test );

function test ()
{
   for( var i = 0; i < clnum; i++ )
   {
      var clname = COMMCLNAME + "_sdbexprt13567_" + i;
      var cl = commCreateCL( db, csname, clname );
      cl.insert( doc );
      clnames.push( clname );
   }

   testExcludeCsCl();

   for( var i = 0; i < clnum; i++ )
   {
      commDropCL( db, csname, clnames[i] );
   }
}

function testExcludeCsCl ()
{
   var csvDir = tmpFileDir + "13567/";
   cmd.run( "mkdir -p " + csvDir );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --dir " + csvDir +
      " --type csv" +
      " --force true";
   command += " --cscl " + csname;
   command += " --excludecscl " + csname + "." + clnames[0] + "," +
      csname + "." + clnames[1];
   testRunCommand( command );

   for( var i = 0; i < clnum; i++ )
   {
      var filename = csvDir + csname + "." + clnames[i] + ".csv";
      if( i == 0 || i == 1 )
         checkFileExist( filename, false );
      else
         checkFileContent( filename, csvContent );
   }

   cmd.run( "rm -rf " + csvDir );
}