/*******************************************************************
* @Description : test export with --genconf
*                seqDB-13612:指定配置文件导出数据，配置文件不存在
*                seqDB-13613:指定配置文件导出数据，配置文件权限错误
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13612";

main( test );

function test ()
{
   var docs = [{ a: 1, b: 1 }];
   var cl = commCreateCL( db, csname, clname );
   cl.insert( docs );

   testExprtGenconf1();  // test export with not exist conf file
   testExprtGenconf2();  // test export with no permission conf file

   commDropCL( db, csname, clname );
}

function testExprtGenconf1 ()
{
   var conffile = tmpFileDir + "sdbexprt13612.conf";
   cmd.run( "rm -rf " + conffile );

   // export with not exist conf file
   var csvfile = tmpFileDir + "sdbexprt13612.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installPath + "bin/sdbexprt" +
      " --file " + csvfile +
      " --conf " + conffile;
   testRunCommand( command, 8 );

   cmd.run( "rm -rf " + conffile );
   cmd.run( "rm -rf " + csvfile );
}

function testExprtGenconf2 ()
{
   var user = getCurrentUser();
   if( user === "root" )
   {
      return;
   }

   var conffile = tmpFileDir + "sdbexprt13613.conf";
   cmd.run( "rm -rf " + conffile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --genconf " + conffile +
      " --fields a,b" +
      " --type csv";
   testRunCommand( command );
   File.chmod( conffile, 0000 );

   var csvfile = tmpFileDir + "sdbexprt13613.csv";
   cmd.run( "rm -rf " + csvfile );
   command = installPath + "bin/sdbexprt" +
      " --file " + csvfile +
      " --conf " + conffile;
   testRunCommand( command, 129 );

   cmd.run( "rm -rf " + conffile );
   cmd.run( "rm -rf " + csvfile );
}