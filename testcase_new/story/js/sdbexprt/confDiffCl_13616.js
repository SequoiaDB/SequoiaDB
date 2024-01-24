/*******************************************************************
* @Description : test export with --conf
*                seqDB-13616:指定配置文件和命令行同时导出不同集合
*                            相同字段数据
*                seqDB-13617:指定配置文件和命令行同时导出不同集合
*                            不同字段数据                 
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME + "_sdbexprt13616";
var clname = COMMCLNAME + "_sdbexprt13616_1";
var clname1 = COMMCLNAME + "_sdbexprt13616_2";

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );
   cl.insert( { a: 1, b: 1 } );
   cl1.insert( { a: 2, b: 2 } );

   testExprtConf1();  // test export with diff cl same fields
   testExprtConf2();  // test export with diff cl diff fields

   commDropCS( db, csname );
}

function testExprtConf1 ()
{
   // export to conf with fields cl:a
   var conffile = tmpFileDir + "sdbexprt13616.conf";
   cmd.run( "rm -rf " + conffile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --cscl " + csname +
      " --fields " + csname + "." + clname + ":a" +
      " --type csv" +
      " --genconf " + conffile;
   testRunCommand( command );

   // export with conf file and fields cl1:a
   var csvDir = tmpFileDir + "13616/";
   cmd.run( "rm -rf " + csvDir );
   cmd.run( "mkdir -p " + csvDir );
   command = installPath + "bin/sdbexprt" +
      " --dir " + csvDir +
      " --conf " + conffile +
      " --fields " + csname + "." + clname1 + ":a";
   testRunCommand( command );

   var csvfile = csvDir + csname + "." + clname + ".csv";
   var content = "a\n1\n";
   checkFileContent( csvfile, content );
   csvfile = csvDir + csname + "." + clname1 + ".csv";
   content = "a\n2\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + conffile );
   cmd.run( "rm -rf " + csvDir );
}

function testExprtConf2 ()
{
   // export to conf with fields cl:a
   var conffile = tmpFileDir + "sdbexprt13617.conf";
   cmd.run( "rm -rf " + conffile );
   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " --cscl " + csname +
      " --genconf " + conffile +
      " --fields " + csname + "." + clname + ":a" +
      " --type csv";
   testRunCommand( command );

   // export with conf and fields cl1:b
   var csvDir = tmpFileDir + "13617/";
   cmd.run( "rm -rf " + csvDir );
   cmd.run( "mkdir -p " + csvDir );
   command = installPath + "bin/sdbexprt" +
      " --dir " + csvDir +
      " --conf " + conffile +
      " --fields " + csname + "." + clname1 + ":b";
   testRunCommand( command );

   var csvfile = csvDir + csname + "." + clname + ".csv";
   var content = "a\n1\n";
   checkFileContent( csvfile, content );
   csvfile = csvDir + csname + "." + clname1 + ".csv";
   content = "b\n2\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + conffile );
   cmd.run( "rm -rf " + csvDir );
}