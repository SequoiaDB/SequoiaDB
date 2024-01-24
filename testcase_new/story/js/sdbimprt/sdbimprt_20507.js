/***************************************************************************
@Description :  seqDB-20507:sdbimprt 的decimal类型支持空字符串
@Modify list :
2020-03-12  chensiqin  Create
****************************************************************************/

testConf.csName = COMMCSNAME;
testConf.clName = COMMCLNAME + "_20507";

main( test );

function test ( testPara )
{

   cl = testPara.testCL;
   clName = testConf.clName
   testImprtCsv( clName, cl );
}

function testImprtCsv ( clName, cl )
{
   var filename = tmpFileDir + "20507.csv";
   var file = fileInit( filename );
   file.write( "123.4, 1\n" );
   file.write( "\"123.4\",2\n" );
   file.write( "\"\",3\n" );
   file.write( ",4" );
   file.close();
   var command = installDir + "bin/sdbimprt" +
      " --hosts " + COORDHOSTNAME + ":" + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName +
      " --type csv " +
      "--file " + filename;

   cl.remove();
   cmd.run( command + " --fields 'a decimal, b int' " );
   var expectResult = [{ "a": { "$decimal": "123.4" }, "b": 1 }, { "a": { "$decimal": "123.4" }, "b": 2 }, { "a": null, "b": 3 }, { "b": 4 }];
   commCompareResults( cl.find().sort( { "b": 1 } ), expectResult );

   cl.remove();
   cmd.run( command + " --fields 'a decimal default 123, b int' " );
   expectResult = [{ "a": { "$decimal": "123.4" }, "b": 1 }, { "a": { "$decimal": "123.4" }, "b": 2 }, { "a": { "$decimal": "123" }, "b": 3 }, { "a": { "$decimal": "123" }, "b": 4 }];
   commCompareResults( cl.find().sort( { "b": 1 } ), expectResult );
}
