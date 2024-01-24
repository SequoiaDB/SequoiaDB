/*******************************************************************
* @Description : test export with -s -p -u -w
*                seqDB-13488:sdb开启鉴权，指定-s-p-u-w导出数据
*                
* @author      : Liang XueWang 
*******************************************************************/
var username = "sequoiadb";
var password = "sequoiadb";
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13488";
var doc = { a: 1 };
var csvContent = "a\n1\n";
var jsonContent = "{ \"a\": 1 }\n";

main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   try
   {
      db.createUsr( username, password );
      var cl = commCreateCL( db, csname, clname );
      cl.insert( doc );
      testExprtCsv();
      testExprtJson();
      commDropCL( db, csname, clname );
   }
   finally
   {
      db.dropUsr( username, password );
   }
}

function testExprtCsv ()
{
   var csvfile = tmpFileDir + "sdbexprt13488.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -u " + username +
      " -w " + password +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields a";
   testRunCommand( command );

   checkFileContent( csvfile, csvContent );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtJson ()
{
   var jsonfile = tmpFileDir + "sdbexprt13488.json";
   cmd.run( "rm -rf " + jsonfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -u " + username +
      " -w " + password +
      " -c " + csname +
      " -l " + clname +
      " --type json" +
      " --file " + jsonfile +
      " --fields a";
   testRunCommand( command );

   checkFileContent( jsonfile, jsonContent );

   cmd.run( "rm -rf " + jsonfile );
}