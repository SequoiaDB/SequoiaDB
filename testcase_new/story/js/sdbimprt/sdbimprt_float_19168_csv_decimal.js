/************************************************************************
*@Description:  seqDB-19168:科学计数法，底数为整数（如 1E+308）
*@Author     :  2019-8-21  huangxiaoni
************************************************************************/

main( test );

function test ()
{
   var type = 'csv';
   var tmpPrefix = "sdbimprt_decimal_19168";
   var csName = COMMCSNAME;
   var clName = tmpPrefix + "_" + type;
   var cl = readyCL( csName, clName );
   var findCond = { "b": { "$type": 2, "$et": "decimal" } };
   var importFile = tmpFileDir + tmpPrefix + "." + type;
   var importFields = 'a int, b decimal';

   // test point1, b value e.g: 1E+0, 1E+1 ...... 1E+309
   // init import file and expect records
   var recsNum = initImportFile_testPoint1( importFile );
   var expRecs = initExpectData_testPoint1( recsNum );
   // import
   var rc = importData( csName, clName, importFile, type, importFields, true );
   // check results
   checkImportRC( rc, recsNum );
   checkCLData( cl, recsNum, expRecs, findCond );
   // clean data
   cmd.run( "rm -rf " + importFile );
   cl.truncate();

   // test poin2, b value e.g: 1E+308, 11E+307 ...... xxxE+0
   // init import file and expect records
   var recsNum = initImportFile_testPoint2( importFile );
   var expRecs = initExpectData_testPoint2( recsNum );
   // import
   var rc = importData( csName, clName, importFile, type, importFields, true );
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
   // test point1, b value e.g: 1E+0, 1E+1 ...... 1E+309
   var recordsNum;
   var str = "";
   var aVal = 0;
   var bValBaseNum = 1;
   var bValBaseIndexNum = 0;
   var bValMaxIndexNum = 309;
   recordsNum = bValMaxIndexNum + 1;
   while( bValBaseIndexNum <= bValMaxIndexNum )
   {
      str += aVal + "," + bValBaseNum + "E+" + bValBaseIndexNum + "\n";
      bValBaseIndexNum++;
      aVal++;
   }
   var file = fileInit( importFile );
   file.write( str );
   file.close();
   return recordsNum;
}

function initImportFile_testPoint2 ( importFile )
{
   // test point2, b value e.g: 1E+400, 11E+309 ...... xxxE+0
   var recordsNum;
   var str = "";
   var aVal = 0;
   var bValBaseNum = 1;
   var bValIndexNum = 400;
   recordsNum = bValIndexNum + 1;
   while( bValIndexNum >= 0 )
   {
      str += aVal + "," + bValBaseNum + "E+" + bValIndexNum + "\n";
      bValBaseNum += "1";
      bValIndexNum--;
      aVal++;
   }
   var file = fileInit( importFile );
   file.write( str );
   file.close();
   return recordsNum;
}

function initExpectData_testPoint1 ( expRecsNum )
{
   var expRecs = [];
   var record = {};
   var tmpBVal = "1";
   for( var i = 0; i < expRecsNum; i++ ) 
   {
      record = { "a": i, "b": { "$decimal": tmpBVal } };
      tmpBVal += "0";
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}

function initExpectData_testPoint2 ( expRecsNum )
{
   var expRecs = [];
   var record = {};
   var bValPrefix = "1";
   var bValSuffix = "0";
   for( var i = 0; i < expRecsNum - 2; i++ ) 
   {
      bValSuffix += "0"
   }

   for( var i = 0; i < expRecsNum; i++ ) 
   {
      bVal = bValPrefix + bValSuffix.substring( 0, bValSuffix.length - i );
      record = { "a": i, "b": { "$decimal": bVal } };
      bValPrefix += "1";
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}