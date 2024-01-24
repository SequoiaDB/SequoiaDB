/******************************************************************************
 * @Description   : seqDB-23716:导入字符串，指定字符串最小最大长度和STRICT（String(min,max)）
 *                : seqDB-23717:导入字符串，指定字符串最大长度和STRICT（String(max,STRICT)）
 *                : seqDB-23718:导入字符串，指定字符串最小长度和STRICT（String(min,0,STRICT)
 * @Author        : liuli
 * @CreateTime    : 2021.03.24
 * @LastEditTime  : 2021.03.24
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_23716";
var filename = tmpFileDir + "23716.csv";

main( test );

function test ( args )
{
   var cl = args.testCL;

   // not specify default, specify min and max
   // strLen < min, errorstop is true
   var fields = "a string(6,8,STRICT)";
   var errorstop = "--errorstop true";
   var expectResult = randomStringAndWriteFile( 5 );
   testImprt( fields, errorstop );
   checkRec( expectResult[0] );

   // min =< strLen <= max, errorstop is true
   var fields = "a string(6,8,STRICT)";
   var errorstop = "--errorstop true";
   var expectResult = randomStringAndWriteFile( 6 );
   testImprt( fields, errorstop );
   commCompareResults( cl.find(), [{ "a": expectResult }] );
   cl.remove();

   var expectResult = randomStringAndWriteFile( 7 );
   testImprt( fields, errorstop );
   commCompareResults( cl.find(), [{ "a": expectResult }] );
   cl.remove();

   var expectResult = randomStringAndWriteFile( 8 );
   testImprt( fields, errorstop );
   commCompareResults( cl.find(), [{ "a": expectResult }] );
   cl.remove();

   // strLen > max, errorstop is true
   var fields = "a string(6,8,STRICT)";
   var errorstop = "--errorstop true";
   var expectResult = randomStringAndWriteFile( 9 );
   testImprt( fields, errorstop );
   checkRec( expectResult[0] );

   // specify default, specify min and max
   // strLen < min, errorstop is false
   var fields = "a string(6,8,STRICT) default ddddd*7";
   var errorstop = "--errorstop false";
   randomStringAndWriteFile( 5 );
   var expectResult = [{ a: "ddddd*7" }];
   testImprt( fields, errorstop );
   commCompareResults( cl.find(), expectResult );
   cl.remove();

   // strLen > max, errorstop is false
   var fields = "a string(6,8,STRICT) default ddddd*7";
   var errorstop = "--errorstop false";
   var expectResult = randomStringAndWriteFile( 9 );
   testImprt( fields, errorstop );
   checkRec( expectResult[0] );

   // strLen < min, default < min
   var fields = "a string(6,8,STRICT) default ddd*5";
   var expectResult = randomStringAndWriteFile( 5 );
   assert.tryThrow( 127, function()
   {
      testImprt( fields );
   } );

   // strLen < min, default > max
   var fields = "a string(6,8,STRICT) default ddddddd*9";
   var expectResult = randomStringAndWriteFile( 5 );
   assert.tryThrow( 127, function()
   {
      testImprt( fields );
   } );

   // not specify default
   // strLen <= max, specify max, not specify min
   var fields = "a string(8,STRICT)";
   var expectResult = randomStringAndWriteFile( 5 );
   testImprt( fields );
   commCompareResults( cl.find(), [{ "a": expectResult }] );
   cl.remove();

   var expectResult = randomStringAndWriteFile( 8 );
   testImprt( fields );
   commCompareResults( cl.find(), [{ "a": expectResult }] );
   cl.remove();

   // strLen > max, specify max, not specify min
   var fields = "a string(8,STRICT)";
   var expectResult = randomStringAndWriteFile( 9 );
   testImprt( fields );
   checkRec( expectResult[0] );

   // strLen < min, specify min, specify max of zero
   var fields = "a string(6,0,STRICT)";
   var expectResult = randomStringAndWriteFile( 5 );
   testImprt( fields );
   checkRec( expectResult[0] );

   // strLen >= min, specify min, specify max of zero
   var fields = "a string(6,0,STRICT)";
   var expectResult = randomStringAndWriteFile( 6 );
   testImprt( fields );
   commCompareResults( cl.find(), [{ "a": expectResult }] );
   cl.remove();

   var expectResult = randomStringAndWriteFile( 7 );
   testImprt( fields );
   commCompareResults( cl.find(), [{ "a": expectResult }] );
   cl.remove();

   // specify default
   // strLen < min, specify min, specify max of zero
   var fields = "a string(6,0,STRICT) default ddd*5";
   var expectResult = randomStringAndWriteFile( 5 );
   assert.tryThrow( 127, function()
   {
      testImprt( fields );
   } );

   cmd.run( "rm -rf " + filename );
}

function testImprt ( fields, errorstop )
{
   var command = installDir + "bin/sdbimprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + testConf.clName +
      " --type csv " +
      errorstop +
      " --fields '" + fields +
      "' --file " + filename;

   cmd.run( command );
}

function checkRec ( expectResult )
{
   var tmpRec = COMMCSNAME + "_" + testConf.clName + "*.rec";
   var rec = cmd.run( "ls " + tmpRec ).split( "\n" )[0];
   var actualResult = cmd.run( "cat " + rec ).split( "\n" )[0];
   if( actualResult != expectResult )
   {
      throw new Error( "actualResult:" + actualResult + "," + "expectResult:" + expectResult );
   }

   // clean tmpRec
   cmd.run( "rm -rf " + tmpRec );
}

function randomStringAndWriteFile ( recsNum )
{
   cmd.run( "rm -rf " + filename );
   var file = fileInit( filename );
   var result = [];
   var str = '';
   for( var i = 0; i < recsNum; i++ )
   {
      str += String.fromCharCode( ( Math.floor( Math.random() * 57 ) + 65 ) );
   }
   file.write( str );
   result.push( str );
   file.close();
   return result;
}