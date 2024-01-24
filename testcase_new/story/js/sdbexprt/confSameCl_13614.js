/*******************************************************************
* @Description : test export with --conf
*                seqDB-13614:指定配置文件和命令行同时导出相同集合
*                            相同字段数据
*                seqDB-13615:指定配置文件和命令行同时导出相同集合
*                            不同字段数据                 
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13614";

main( test );

function test ()
{
   var docs = [{ a: 1, b: 1 }];
   var cl = commCreateCL( db, csname, clname );
   cl.insert( docs );

   testExprtConf1();  // test export with same cl same fields
   testExprtConf2();  // test export with same cl diff fields

   commDropCL( db, csname, clname );
}

function testExprtConf1 ()
{
   // export to conf with fields a
   var conffile = tmpFileDir + "sdbexprt13614.conf";
   cmd.run( "rm -rf " + conffile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --fields a" +
      " --type csv" +
      " --genconf " + conffile;
   testRunCommand( command );

   // export with conf file and fields a
   var csvfile = tmpFileDir + "sdbexprt13614.csv";
   cmd.run( "rm -rf " + csvfile );
   command = installPath + "bin/sdbexprt" +
      " --file " + csvfile +
      " --conf " + conffile +
      " --fields " + csname + "." + clname + ":a";
   testRunCommand( command );

   var content = "a\n1\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + conffile );
   cmd.run( "rm -rf " + csvfile );
}

function testExprtConf2 ()
{
   // export to conf with fields a
   var conffile = tmpFileDir + "sdbexprt13615.conf";
   cmd.run( "rm -rf " + conffile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --genconf " + conffile +
      " --fields a" +
      " --type csv";
   testRunCommand( command );

   // export with conf file and fields b
   var csvfile = tmpFileDir + "sdbexprt13615.csv";
   cmd.run( "rm -rf " + csvfile );
   command = installPath + "bin/sdbexprt" +
      " --file " + csvfile +
      " --conf " + conffile +
      " --fields " + csname + "." + clname + ":b";
   testRunCommand( command );

   var content = "b\n1\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + conffile );
   cmd.run( "rm -rf " + csvfile );
}