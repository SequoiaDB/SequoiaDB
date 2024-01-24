/*******************************************************************
* @Description : test export with --includeregex
*                seqDB-13601:导出完整的正则表达式数据
*                seqDB-13602:只导出正则表达式，不导出选项
*                seqDB-12204:sdbexprt导出csv，指定--includebinary 
*                                                 --includeregex
*                SEQUOIADBMAINSTREAM-490           
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13601";
var clname1 = COMMCLNAME + "_sdbimprt13601";
var docs = [{ "key": Regex( "^W", "i" ) },
{ "key": Regex( "^W", "m" ) },
{ "key": Regex( "^W", "x" ) },
{ "key": Regex( "^W", "s" ) }];

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );
   cl.insert( docs );

   testExprtRegex1();    // test includeregex true
   var expRecs = [
      "{\"key\":{\"$regex\":\"^W\",\"$options\":\"i\"}}",
      "{\"key\":{\"$regex\":\"^W\",\"$options\":\"m\"}}",
      "{\"key\":{\"$regex\":\"^W\",\"$options\":\"x\"}}",
      "{\"key\":{\"$regex\":\"^W\",\"$options\":\"s\"}}"
   ];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cl1.truncate();
   testExprtRegex2();    // test includeregex false
   expRecs = [
      "{\"key\":{\"$regex\":\"^W\",\"$options\":\"\"}}",
      "{\"key\":{\"$regex\":\"^W\",\"$options\":\"\"}}",
      "{\"key\":{\"$regex\":\"^W\",\"$options\":\"\"}}",
      "{\"key\":{\"$regex\":\"^W\",\"$options\":\"\"}}"
   ];
   cursor = cl1.find( {}, { _id: { $include: 0 } } );
   actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}

function testExprtRegex1 ()
{
   var csvfile = tmpFileDir + "sdbexprt13601.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --includeregex true" +
      " --fields key" +
      " --sort '{ _id: 1 }'" +
      " --type csv";
   testRunCommand( command );

   var content = "key\n" +
      "\"/^W/i\"\n" +
      "\"/^W/m\"\n" +
      "\"/^W/x\"\n" +
      "\"/^W/s\"\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --type csv" +
      " --file " + csvfile +
      " --headerline true" +
      " --fields='key regex'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtRegex2 ()
{
   var csvfile = tmpFileDir + "sdbexprt13602.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --includeregex false" +
      " --fields key" +
      " --sort '{ _id: 1 }'" +
      " --type csv";
   testRunCommand( command );

   var content = "key\n" +
      "\"^W\"\n" +
      "\"^W\"\n" +
      "\"^W\"\n" +
      "\"^W\"\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --type csv" +
      " --file " + csvfile +
      " --headerline true" +
      " --fields='key regex'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   cmd.run( "rm -rf " + csvfile );
}