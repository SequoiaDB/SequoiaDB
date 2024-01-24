/******************************************************************************
 * @Description   : seqDB-28507:sdbimprt导入数据时replacekeydup参数检验
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.10.25
 * @LastEditTime  : 2022.10.27
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_28507";

main( test );
function test ( testPara )
{
   cl = testPara.testCL;
   clName = testConf.clName
   testImprtJson( clName, cl );
   cl.remove();
   testImprtCsv( clName, cl );
}
function testImprtJson ( clName, cl )
{
   var filename = tmpFileDir + "28507.json";
   cmd.run( "rm -rf " + filename );
   var file = fileInit( filename );
   file.write( "{ \"a\": 1, \"b\": 2 }" );
   file.close();
   var command = installDir + "bin/sdbimprt" +
      " --hosts " + COORDHOSTNAME + ":" + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName +
      " --type json " +
      "--file " + filename;

   //replacekeydup不指定时，错误值为127
   assert.tryThrow( 127, function()
   {
      cmd.run( command + " --replacekeydup" );
   } );
   //replacekeydup指定为非bool值，错误值为127
   assert.tryThrow( 127, function()
   {
      cmd.run( command + " --replacekeydup=a" );
   } );
   var expectResult = [];
   commCompareResults( cl.find(), expectResult );
   //指定replacekeydup为true
   cmd.run( command + " --replacekeydup=true" );
   expectResult = [{ "a": 1, "b": 2 }];
   commCompareResults( cl.find(), expectResult );

   cmd.run( "rm -rf " + filename );
}

function testImprtCsv ( clName, cl )
{
   var filename = tmpFileDir + "28507.csv";
   cmd.run( "rm -rf " + filename );
   var file = fileInit( filename );
   file.write( '1, 2' );
   file.close();
   var command = installDir + "bin/sdbimprt" +
      " --hosts " + COORDHOSTNAME + ":" + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clName +
      " --type csv " +
      "--fields a,b " +
      "--file " + filename;
   //replacekeydup不指定时，错误值为127
   assert.tryThrow( 127, function()
   {
      cmd.run( command + " --replacekeydup" );
   } );
   //replacekeydup指定为非bool值，错误值为127
   assert.tryThrow( 127, function()
   {
      cmd.run( command + " --replacekeydup=a" );
   } );
   var expectResult = [];
   commCompareResults( cl.find(), expectResult );
   //指定replacekeydup为true
   cmd.run( command + " --replacekeydup=true" );
   expectResult = [{ "a": "1", "b": 2 }];
   commCompareResults( cl.find(), expectResult );

   cmd.run( "rm -rf " + filename );
}