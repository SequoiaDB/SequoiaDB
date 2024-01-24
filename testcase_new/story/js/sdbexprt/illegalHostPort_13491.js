/*******************************************************************
* @Description : test export with illegal hostname svcname
*                seqDB-13491:指定主机名/端口错误          
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13491";
var doc = { a: 1 };

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   cl.insert( doc );
   testExprtIllegalHost();
   testExprtUnusedPort();
   commDropCL( db, csname, clname );
}

function testExprtIllegalHost ()
{
   var csvfile = tmpFileDir + "sdbexprt13491.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " -s abcde" +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields a";
   testRunCommand( command, 135 );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtUnusedPort ()
{
   var jsonfile = tmpFileDir + "sdbexprt13491.json";
   cmd.run( "rm -rf " + jsonfile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + RSRVPORTBEGIN +   // unused port
      " -c " + csname +
      " -l " + clname +
      " --type json" +
      " --file " + jsonfile +
      " --fields a";
   testRunCommand( command, 135 );

   cmd.run( "rm -rf " + jsonfile );
}