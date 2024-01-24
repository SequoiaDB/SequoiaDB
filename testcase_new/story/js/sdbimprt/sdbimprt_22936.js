/************************************
*@Description: seqDB-22936:导入字符串类型的数据，指定字符串最小长度
*@Author:      2020/11/03  liuli
**************************************/
testConf.clName = COMMCLNAME + "_22936";
var filename = tmpFileDir + "22936.csv";

main( test );

function test ( args )
{
   var cl = args.testCL;

   // 随机生成1M长度的字符串
   var strLenRange = [1024 * 1024, 1024 * 1024];
   var csvContent1 = randomStringAndWriteFile( 1, strLenRange );
   var expectResult = getExpectResult( csvContent1, 7 );
   var fields = "a string(7)";
   testImprt( cl, fields, expectResult );

   var expectResult = getExpectResult( csvContent1 );
   var fields = "a string('" + 1024 * 1024 + "')";
   testImprt( cl, fields, expectResult );

   var fields = "a string('" + 1024 * 1024 + 1 + "')";
   testImprt( cl, fields, expectResult );

   // 随机取100~200个字符，随机100次
   var strLenRange = [100, 200];
   var csvContent2 = randomStringAndWriteFile( 100, strLenRange );
   var expectResult = getExpectResult( csvContent2, 150 );
   var fields = "a string(150)";
   testImprt( cl, fields, expectResult );

   var expectResult = getExpectResult( csvContent2, 200 );
   var fields = "a string(200)";
   testImprt( cl, fields, expectResult );

   var expectResult = getExpectResult( csvContent2 );
   var fields = "a string(201)";
   testImprt( cl, fields, expectResult );

   var fields = "a string(0)";
   testImprt( cl, fields, expectResult );

   // 随机生成ascii值，字符串长度100
   var strLenRange = [100, 100];
   var csvContent3 = randomStringAndWriteFile( 1, strLenRange );
   var expectResult = getExpectResult( csvContent3, 20 );
   var fields = "a string(20)";
   testImprt( cl, fields, expectResult );

   var expectResult = getExpectResult( csvContent3 );
   var fields = "a string(100)";
   testImprt( cl, fields, expectResult );

   var fields = "a string(101)";
   testImprt( cl, fields, expectResult );

   // 字符串包含中文
   cmd.run( "rm -rf " + filename );
   var file = fileInit( filename );
   file.write( "1\32abc巨杉def" );
   file.close();
   var expectResult = [{ "a": "abc" }];
   var fields = "a string(3)";
   testImprt( cl, fields, expectResult );
   var expectResult = [];
   var fields = "a string(4)";
   testImprt( cl, fields, expectResult );
   var fields = "a string(5)";
   testImprt( cl, fields, expectResult );
   var expectResult = [{ "a": "abc巨" }];
   var fields = "a string(6)";
   testImprt( cl, fields, expectResult );

   cl.remove();
   cmd.run( "rm -rf " + filename );

   // 空字符串
   var file = fileInit( filename );
   file.write( '1,""' );
   file.close();
   var expectResult = [{ "a": "" }];
   var fields = "a string(0)";
   testImprt( cl, fields, expectResult, "" );

   var fields = "a string(5)";
   testImprt( cl, fields, expectResult, "" );

   cl.remove();
   cmd.run( "rm -rf " + filename );
   cmd.run( "rm -rf " + COMMCSNAME + "_" + testConf.clName + "*.rec" );
}

function testImprt ( cl, fields, expectResult, delimiterStr )
{
   if( typeof ( delimiterStr ) == "undefined" ) { delimiterStr = " -a '' -e '\32'"; }
   var command = installDir + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + testConf.clName +
      " --type csv " +
      "--fields '_id int, " + fields +
      "' --file " + filename +
      delimiterStr;

   cmd.run( command );
   commCompareResults( cl.find().sort( { "_id": -1 } ), expectResult );
   cl.remove();
}

function getExpectResult ( csvContent, cutoff )
{
   var expectResult = [];
   for( var i = 0; i < csvContent.length; i++ )
   {
      if( typeof ( cutoff ) != "undefined" )
      {
         expectResult.push( { "a": csvContent[i].slice( 0, cutoff ) } );
      }
      else
      {
         expectResult.push( { "a": csvContent[i] } );
      }
   }
   return expectResult;
}

function randomStringAndWriteFile ( recsNum, strLenRange )
{
   cmd.run( "rm -rf " + filename );
   var file = fileInit( filename );
   var result = [];
   for( var i = recsNum; i > 0; i-- )
   {
      file.write( i + "" + "\32" );
      var str = '';
      for( var j = ( Math.random() * ( strLenRange[1] - strLenRange[0] ) + strLenRange[0] ); j > 0; --j )
      {
         str += String.fromCharCode( ( Math.floor( Math.random() * 94 ) + 33 ) );
      }
      file.write( str + "\n" );
      result.push( str );
   }
   file.close();
   return result;
}