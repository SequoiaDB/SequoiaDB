/*******************************************************************
* @Description : test export with -u -w, no user
*                seqDB-13492:sdb未开启鉴权，指定任意用户/密码         
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13492";
var doc = { a: 1 };
var csvContent = "a\n1\n";

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   cl.insert( doc );
   testExprtWithUser();
   commDropCL( db, csname, clname );
}

function testExprtWithUser ()
{
   var csvfile = tmpFileDir + "sdbexprt13492.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -u sequoiadb" +
      " -w sequoiadb" +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields a";
   testRunCommand( command );

   checkFileContent( csvfile, csvContent );

   cmd.run( "rm -rf " + csvfile );
}