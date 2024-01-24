/************************************************************************
*@Description:  seqDB-19168:科学计数法，底数为整数（如 1E+308）
*@Author     :  2019-8-21  huangxiaoni
************************************************************************/

main( test );

function test ()
{
   var type = 'json';
   var tmpPrefix = "sdbimprt_int_19168";
   var csName = COMMCSNAME;
   var clName = tmpPrefix + "_" + type;
   var cl = readyCL( csName, clName );
   var importFile = tmpFileDir + tmpPrefix + "." + type;

   // test point1, b value e.g: 1E+10, 1E+9 ...... 1E+0
   // init import file and expect records
   var recsNum = initImportFile_testPoint1( importFile );
   var expRecs = initExpectData_testPoint1( recsNum );
   // import
   var rc = importData( csName, clName, importFile, type );
   // check results
   checkImportRC( rc, recsNum );
   var findCond = { "b": { "$type": 2, "$et": "double" } };
   checkCLData( cl, recsNum, expRecs, findCond );
   // clean data
   cmd.run( "rm -rf " + importFile );
   cl.truncate();

   // test poin2, b value e.g: 1E+308, 11E+307 ...... xxxE+0
   // init import file and expect records
   var recsNum = initImportFile_testPoint2( importFile );
   var expRecs = initExpectData_testPoint2( recsNum );
   // import
   var rc = importData( csName, clName, importFile, type );
   // check results
   checkImportRC( rc, recsNum );
   // b: double
   var findType = "double";
   var findCond = { "b": { "$type": 2, "$et": findType } };
   var expRecsNum = 19;
   var newExpRecs = JSON.stringify( JSON.parse( expRecs ).slice( 0, 19 ) );
   checkCLData( cl, expRecsNum, newExpRecs, findCond, findType );
   // b: decimal
   var findType = "decimal";
   var findCond = { "b": { "$type": 2, "$et": findType } };
   var expRecsNum = recsNum - 19;
   var newExpRecs = JSON.stringify( JSON.parse( expRecs ).slice( 19 ) );
   checkCLData( cl, expRecsNum, newExpRecs, findCond, findType );
   // clean data
   cmd.run( "rm -rf " + importFile );
   cl.truncate();

   // test poin3, bValIndexNum > 308, b value e.g: 1E+309
   // init import file and expect records
   var recsNum = initImportFile_testPoint3( importFile );
   var expRecs = initExpectData_testPoint3( recsNum );
   // import
   var rc = importData( csName, clName, importFile, type );
   // check results
   checkImportRC( rc, recsNum );
   checkCLData( cl, recsNum, expRecs, findCond );
   // clean data
   cmd.run( "rm -rf " + importFile );
   cl.truncate();

   cleanCL( csName, clName );
}

function initImportFile_testPoint1 ( importFile )
{
   // test point1, b value e.g: 1E+10, 1E+9 ...... 1E+0
   var recordsNum;
   var str = "";
   var aVal = 0;
   var bValBaseNum = 1;
   var bValIndexNum = 10; //b value < 2147483647
   recordsNum = bValIndexNum + 1;
   while( bValIndexNum >= 0 )
   {
      str += "{a:" + aVal + ",b:" + bValBaseNum + "E+" + bValIndexNum + "}\n";
      bValIndexNum--;
      aVal++;
   }
   var file = fileInit( importFile );
   file.write( str );
   file.close();
   return recordsNum;
}

function initImportFile_testPoint2 ( importFile )
{
   // test point2, b value e.g: 1E+308, 11E+307 ...... xxxE+0
   var recordsNum;
   var str = "";
   var aVal = 0;
   var bValBaseNum = 1;
   var bValIndexNum = 308;
   recordsNum = bValIndexNum + 1;
   while( bValIndexNum >= 0 )
   {
      str += "{a:" + aVal + ",b:" + bValBaseNum + "E+" + bValIndexNum + "}\n";
      bValBaseNum += "1";
      bValIndexNum--;
      aVal++;
   }
   var file = fileInit( importFile );
   file.write( str );
   file.close();
   return recordsNum;
}

function initImportFile_testPoint3 ( importFile )
{
   // test point3, b value e.g: bValIndexNum > 308, b value e.g: 1E+309
   var recordsNum = 1;
   var aVal = 0;
   var bValBaseNum = 1;
   var bValIndexNum = 309;
   var str = "{a:" + aVal + ",b:" + bValBaseNum + "E+" + bValIndexNum + "}\n";
   var file = fileInit( importFile );
   file.write( str );
   file.close();
   return recordsNum;
}

function initExpectData_testPoint1 ( expRecsNum )
{
   var expRecs = [];
   var record = {};
   var tmpBVal = 1E+10;
   for( var i = 0; i < expRecsNum; i++ ) 
   {
      record = { "a": i, "b": tmpBVal };
      tmpBVal = tmpBVal / 10;
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}

function initExpectData_testPoint2 ( expRecsNum )
{
   var expRecs = [];
   var record = {};
   var tmpBVal1 = "1.1";

   var tmpBVal2Prefix = "";
   var tmpBVal2PrefixLen = 20; // number of 1, e.g: "111......"
   for( var i = 0; i < tmpBVal2PrefixLen; i++ ) 
   {
      tmpBVal2Prefix += "1";
   }

   var tmpBVal2Suffix = ""; // number of 0, e.g: "000......"
   for( var i = 0; i < expRecsNum - tmpBVal2PrefixLen; i++ ) 
   {
      tmpBVal2Suffix += "0";
   }

   for( var i = 0; i < expRecsNum; i++ ) 
   {
      if( i === 0 ) 
      {
         record = { "a": i, "b": 1e+308 };
      }
      else if( i > 0 && i < 15 )
      {
         record = { "a": i, "b": Number( tmpBVal1 + "e+308" ) };
         tmpBVal1 += "1";
      }
      else if( i >= 15 && i < 19 )
      {
         record = { "a": i, "b": 1.111111111111111e+308 };
      }
      else if( i >= 19 && i < expRecsNum )
      {
         var bVal = tmpBVal2Prefix + tmpBVal2Suffix.substring( 0, tmpBVal2Suffix.length - ( i - 19 ) );
         record = { "a": i, "b": { "$decimal": bVal } };
         tmpBVal2Prefix += "1";
      }
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}

function initExpectData_testPoint3 ( expRecsNum )
{
   var expRecs = [];
   var bVal = "1"; // number of 0, e.g: "1000......"
   for( var i = 0; i < 309; i++ ) 
   {
      bVal += "0";
   }

   var record = { "a": 0, "b": { "$decimal": bVal } };
   expRecs.push( JSON.stringify( record ) );
   return "[" + expRecs + "]";
}