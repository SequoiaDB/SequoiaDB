/*******************************************************************
* @Description : test export with --genconf --genfields
*                seqDB-13610:指定输出到配置文件，并对每个集合生成field
*                seqDB-13611:指定输出到配置文件，不对集合生成fields
*                seqDB-10942:指定kicknull参数生成conf文件并
*                            指定conf文件导出数据
* @author      : Liang XueWang 
*
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13610";

main( test );

function test ()
{
   var docs = [{ a: 1, b: 1 }];
   var cl = commCreateCL( db, csname, clname );
   cl.insert( docs );

   testExprtGenconf1();  // test genconf with genfields true
   testExprtGenconf2();  // test genconf with genfields false

   commDropCL( db, csname, clname );
}

function testExprtGenconf1 ()
{
   var conffile = tmpFileDir + "sdbexprt13610.conf";
   cmd.run( "rm -rf " + conffile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --genconf " + conffile +
      " --genfields true" +
      " --fields a,b" +
      " --type csv";
   testRunCommand( command );

   var content = "hosts = " + COORDHOSTNAME + ":" + COORDSVCNAME + "\n" +
      "user = \n" +
      "type = csv\n" +
      "ssl = false\n" +
      "floatfmt = %.16g\n" +
      "delchar = \"\n" +
      "delfield = ,\n" +
      "included = true\n" +
      "includebinary = false\n" +
      "includeregex = false\n" +
      "kicknull = false\n" +
      "checkdelimeter = true\n" +
      "csname = " + csname + "\n" +
      "clname = " + clname + "\n" +
      "fields = " + csname + "." + clname + ":a,b\n";
   checkFileContent( conffile, content );

   var csvfile = tmpFileDir + "sdbexprt13610.csv";
   cmd.run( "rm -rf " + csvfile );
   command = installPath + "bin/sdbexprt" +
      " --file " + csvfile +
      " --conf " + conffile;
   testRunCommand( command );
   content = "a,b\n1,1\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + conffile );
   cmd.run( "rm -rf " + csvfile );
}

function testExprtGenconf2 ()
{
   var conffile = tmpFileDir + "sdbexprt13611.conf";
   cmd.run( "rm -rf " + conffile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --genconf " + conffile +
      " --genfields false" +
      " --fields a,b" +
      " --type csv";
   testRunCommand( command );

   var content = "hosts = " + COORDHOSTNAME + ":" + COORDSVCNAME + "\n" +
      "user = \n" +
      "type = csv\n" +
      "ssl = false\n" +
      "floatfmt = %.16g\n" +
      "delchar = \"\n" +
      "delfield = ,\n" +
      "included = true\n" +
      "includebinary = false\n" +
      "includeregex = false\n" +
      "kicknull = false\n" +
      "checkdelimeter = true\n" +
      "csname = " + csname + "\n" +
      "clname = " + clname + "\n";
   checkFileContent( conffile, content );

   var csvfile = tmpFileDir + "sdbexprt13611.csv";
   cmd.run( "rm -rf " + csvfile );
   command = installPath + "bin/sdbexprt" +
      " --file " + csvfile +
      " --fields a,b" +
      " --conf " + conffile;
   testRunCommand( command );
   content = "a,b\n1,1\n";
   checkFileContent( csvfile, content );

   cmd.run( "rm -rf " + conffile );
   cmd.run( "rm -rf " + csvfile );
}