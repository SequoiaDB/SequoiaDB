/*******************************************************************
* @Description : test export with --includebinary
*                seqDB-13599:导出完整的二进制数据
*                seqDB-13600:只导出二进制内容，不导出类型
*                seqDB-12204:sdbexprt导出csv，指定--includebinary 
*                                                 --includeregex
*                SEQUOIADBMAINSTREAM-490                
* @author      : Liang XueWang
* 
*******************************************************************/
var csname = COMMCSNAME;
var clname = COMMCLNAME + "_sdbexprt13599";
var clname1 = COMMCLNAME + "_sdbimprt13599";
var docs = [{ "key": BinData( "aGVsbG8gd29ybGQ=", "0" ) },
{ "key": BinData( "aGVsbG8gd29ybGQ=", "1" ) },
{ "key": BinData( "aGVsbG8gd29ybGQ=", "2" ) },
{ "key": BinData( "aGVsbG8gd29ybGQ=", "3" ) },
{ "key": BinData( "aGVsbG8gd29ybGQ=", "5" ) },
{ "key": BinData( "aGVsbG8gd29ybGQ=", "128" ) }];

main( test );

function test ()
{
   var cl = commCreateCL( db, csname, clname );
   var cl1 = commCreateCL( db, csname, clname1 );
   cl.insert( docs );

   testExprtBinary1();    // test includebinary true
   var expRecs = [
      "{\"key\":{\"$binary\":\"aGVsbG8gd29ybGQ=\",\"$type\":\"0\"}}",
      "{\"key\":{\"$binary\":\"aGVsbG8gd29ybGQ=\",\"$type\":\"1\"}}",
      "{\"key\":{\"$binary\":\"aGVsbG8gd29ybGQ=\",\"$type\":\"2\"}}",
      "{\"key\":{\"$binary\":\"aGVsbG8gd29ybGQ=\",\"$type\":\"3\"}}",
      "{\"key\":{\"$binary\":\"aGVsbG8gd29ybGQ=\",\"$type\":\"5\"}}",
      "{\"key\":{\"$binary\":\"aGVsbG8gd29ybGQ=\",\"$type\":\"128\"}}"
   ];
   var cursor = cl1.find( {}, { _id: { $include: 0 } } );
   var actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   cl1.truncate();
   testExprtBinary2();    // test includebinary false
   expRecs = [
      "{\"key\":{\"$binary\":\"aGVsbG8gd29ybGQ=\",\"$type\":\"0\"}}",
      "{\"key\":{\"$binary\":\"aGVsbG8gd29ybGQ=\",\"$type\":\"0\"}}",
      "{\"key\":{\"$binary\":\"aGVsbG8gd29ybGQ=\",\"$type\":\"0\"}}",
      "{\"key\":{\"$binary\":\"aGVsbG8gd29ybGQ=\",\"$type\":\"0\"}}",
      "{\"key\":{\"$binary\":\"aGVsbG8gd29ybGQ=\",\"$type\":\"0\"}}",
      "{\"key\":{\"$binary\":\"aGVsbG8gd29ybGQ=\",\"$type\":\"0\"}}"
   ];
   cursor = cl1.find( {}, { _id: { $include: 0 } } );
   actRecs = getRecords( cursor );
   checkRecords( expRecs, actRecs );

   commDropCL( db, csname, clname );
   commDropCL( db, csname, clname1 );
}

function testExprtBinary1 ()
{
   var csvfile = tmpFileDir + "sdbexprt13599.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --includebinary true" +
      " --fields key" +
      " --sort '{ _id: 1 }'" +
      " --type csv";
   testRunCommand( command );

   var content = "key\n" +
      "\"(0)aGVsbG8gd29ybGQ=\"\n" +
      "\"(1)aGVsbG8gd29ybGQ=\"\n" +
      "\"(2)aGVsbG8gd29ybGQ=\"\n" +
      "\"(3)aGVsbG8gd29ybGQ=\"\n" +
      "\"(5)aGVsbG8gd29ybGQ=\"\n" +
      "\"(128)aGVsbG8gd29ybGQ=\"\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --type csv" +
      " --file " + csvfile +
      " --headerline true" +
      " --fields='key binary'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   cmd.run( "rm -rf " + csvfile );
}

function testExprtBinary2 ()
{
   var csvfile = tmpFileDir + "sdbexprt13600.csv";
   cmd.run( "rm -rf " + csvfile );

   var command = installPath + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname +
      " --file " + csvfile +
      " --includebinary false" +
      " --fields key" +
      " --sort '{ _id: 1 }'" +
      " --type csv";
   testRunCommand( command );

   var content = "key\n" +
      "\"aGVsbG8gd29ybGQ=\"\n" +
      "\"aGVsbG8gd29ybGQ=\"\n" +
      "\"aGVsbG8gd29ybGQ=\"\n" +
      "\"aGVsbG8gd29ybGQ=\"\n" +
      "\"aGVsbG8gd29ybGQ=\"\n" +
      "\"aGVsbG8gd29ybGQ=\"\n";
   checkFileContent( csvfile, content );

   command = installPath + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + csname +
      " -l " + clname1 +
      " --type csv" +
      " --file " + csvfile +
      " --headerline true" +
      " --fields='key binary'" +
      " --parsers 1 -j 1";
   testRunCommand( command );

   cmd.run( "rm -rf " + csvfile );
}