/*******************************************************************
* @Description : test export with --excludecscl
*                seqDB-13570:--excludecscl指定的集合在数据库中不存在              
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
      var csname = COMMCSNAME + "_sdbexprt13570_" + i;
      for( var j = 0; j < clnum; j++ )
      {
         var clname = COMMCLNAME + "_sdbexprt13570_" + j;
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
   var csvDir = tmpFileDir + "13570/";
   cmd.run( "mkdir -p " + csvDir );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --dir " + csvDir +
      " --type csv" +
      " --force true";
   command += " --cscl ";
   for( var i = 0; i < csnum; i++ )
   {
      command += csnames[i];
      if( i !== csnum - 1 )
         command += ",";
   }
   // exclude not exist cl 
   command += " --excludecscl " + csnames[0] + ".notExistCl";
   testRunCommand( command );

   for( var i = 0; i < csnum; i++ )
   {
      for( var j = 0; j < clnum; j++ )
      {
         var filename = csvDir + csnames[i] + "." + clnames[j] + ".csv";
         checkFileContent( filename, csvContent );
      }
   }

   cmd.run( "rm -rf " + csvDir );
}