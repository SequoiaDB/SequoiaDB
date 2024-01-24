/***************************************************************************
@Description : seqDB-21935:指定 linepriority默认、auto、true、false情况下导入记录优先级正确性
@Modify list :
2020-03-11  chensiqin  Create
****************************************************************************/
testConf.clName = COMMCLNAME + "_21935";

main( test );

function test ( testPara )
{
   testImprtJson1( testPara.testCL );
   testImprtJson2( testPara.testCL );
   testImprtCsv1( testPara.testCL );
   testImprtCsv2( testPara.testCL );

   // clean *.rec file
   var tmpRec = COMMCSNAME + "_" + testConf.clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );
}

function testImprtJson1 ( cl )
{
   var filename = tmpFileDir + "21935_1.json";
   var file = fileInit( filename );
   file.write( "{ \"a\": 1, \"c\": \"csq\"csq\" }\n" );
   file.write( "{ \"a\": 1, \"b\": 2 }" );
   file.close();
   var command = installDir + "bin/sdbimprt" +
      " --hosts " + COORDHOSTNAME + ":" + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + testConf.clName +
      " --type json " +
      "--fields a,b,c " +
      "--file " + filename;

   //不指定--linepriority 
   cmd.run( command );
   var expectResult = [];
   commCompareResults( cl.find(), expectResult );

   //linepriority auto
   cmd.run( command + " --linepriority auto" );
   commCompareResults( cl.find(), expectResult );

   //linepriority flase
   cmd.run( command + " --linepriority false" );
   commCompareResults( cl.find(), expectResult );

   // linepriority true
   cmd.run( command + " --linepriority true" );
   expectResult = [{ "a": 1, "b": 2 }];
   commCompareResults( cl.find(), expectResult );
}

function testImprtJson2 ( cl )
{
   var filename = tmpFileDir + "21935_2.json";
   var file = fileInit( filename );
   file.write( "{ \"_id\": 1, \"a\": \"Mike\n\" }\n" );
   file.write( "{ \"_id\": 2, \"a\": 1, \"b\": 2 }" );
   file.close();
   var command = installDir + "bin/sdbimprt" +
      " --hosts " + COORDHOSTNAME + ":" + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + testConf.clName +
      " --type json " +
      "--fields a,b,c " +
      "--file " + filename;

   //不指定--linepriority 
   cl.remove();
   cmd.run( command );
   var expectResult = [{ "a": "Mike\n" }, { "a": 1, "b": 2 }];
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );

   //linepriority auto
   cl.remove();
   cmd.run( command + " --linepriority auto" );
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );

   //linepriority flase
   cl.remove();
   cmd.run( command + " --linepriority false" );
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );

   // linepriority true
   cl.remove();
   cmd.run( command + " --linepriority true" );
   expectResult = [{ "a": 1, "b": 2 }];
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );
}

function testImprtCsv1 ( cl )
{
   var filename = tmpFileDir + "21935_1.csv";
   var file = fileInit( filename );
   file.write( "\"Jack\",18,\"Chi\"na\"\n" );
   file.write( "\"Mike\",20,\"USA\"\n" );
   file.close();
   var command = installDir + "bin/sdbimprt" +
      " --hosts " + COORDHOSTNAME + ":" + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + testConf.clName +
      " --type csv " +
      "--fields a,b,c " +
      "--file " + filename;

   //不指定--linepriority 
   cl.remove();
   cmd.run( command );
   var expectResult = [{ "a": "Mike", "b": 20, "c": "USA" }];
   commCompareResults( cl.find(), expectResult );
   cl.remove();

   //linepriority auto
   cmd.run( command + " --linepriority auto" );
   commCompareResults( cl.find(), expectResult );
   cl.remove();

   //linepriority true 
   cmd.run( command + " --linepriority true" );
   commCompareResults( cl.find(), expectResult );
   cl.remove();

   // linepriority false
   cmd.run( command + " --linepriority false" );
   expectResult = [];
   commCompareResults( cl.find(), expectResult );
}

function testImprtCsv2 ( cl )
{
   var filename = tmpFileDir + "21935_2.csv";
   var file = fileInit( filename );
   file.write( '1,"Jack",18,"Chi\n' );
   file.write( '2,na"\n' );
   file.write( '3,"Mike",20,"USA"\n' );
   file.close();
   var command = installDir + "bin/sdbimprt" +
      " --hosts " + COORDHOSTNAME + ":" + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + testConf.clName +
      " --type csv " +
      "--fields _id,a,b,c " +
      "--file " + filename;

   //不指定--linepriority 
   cl.remove();
   cmd.run( command );
   var expectResult = [{ "a": "Jack", "b": 18, "c": "Chi" }, { "a": "na" }, { "a": "Mike", "b": 20, "c": "USA" }];
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );
   cl.remove();

   //linepriority auto
   cmd.run( command + " --linepriority auto" );
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );
   cl.remove();

   //linepriority false
   cmd.run( command + " --linepriority true" );
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );
   cl.remove();

   // linepriority true
   cmd.run( command + " --linepriority false" );
   expectResult = [{ "a": "Jack", "b": 18, "c": "Chi\n2,na" }, { "a": "Mike", "b": 20, "c": "USA" }];
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );
}
