/***************************************************************************
@Description : seqDB-22066:指定 --decimalto 参数进行导入
@Modify list :
2020-04-08  chensiqin  Create
****************************************************************************/

testConf.csName = COMMCSNAME;
testConf.clName = COMMCLNAME + "_22066";

main( test );

function test ( testPara )
{

   cl = testPara.testCL;
   clName = testConf.clName
   testImprtJson( clName, cl );
}

function testImprtJson ( clName, cl )
{

   var filename = tmpFileDir + "22066.json";
   var file = fileInit( filename );
   file.write( "{ \"_id\": 1, \"a\" : 12345.12345678987654321 }\n" );
   file.write( "{ \"_id\": 2, \"b\" : { \"$decimal\": 123 } }" );
   file.close();
   var command = installDir + "bin/sdbimprt" +
      " --hosts " + COORDHOSTNAME + ":" + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName +
      " --type json " +
      "--file " + filename;

   //不指定--decimalto 
   cmd.run( command );
   var expectResult = [{ "a": { "$decimal": "12345.12345678987654321" } }, { "b": { "$decimal": "123" } }];
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );

   //decimalto ""
   cl.remove();
   cmd.run( command + " --decimalto ''" );
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );

   //decimalto double
   cl.remove();
   expectResult = [{ "a": 12345.12345678988 }, { "b": 123 }];
   cmd.run( command + " --decimalto double" );
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );

   // decimalto string
   cl.remove();
   cmd.run( command + " --decimalto string" );
   expectResult = [{ "a": "12345.12345678987654321" }, { "b": "123" }];
   commCompareResults( cl.find().sort( { "_id": 1 } ), expectResult );

   // decimalto
   cl.remove();
   assert.tryThrow( 127, function()
   {
      cmd.run( command + " --decimalto" );
   } )
}
