/************************************************************************
*@Description:  seqDB-19172:科学计数法，底数为小数点+小数位，小数位包含有效数字（.001E+310）
         import type: decimal
*@Author     :  2019-8-21  huangxiaoni
************************************************************************/


main( test );

function test ()
{
   var type = 'csv';
   var tmpPrefix = "sdbimprt_decimal_19172";
   var csName = COMMCSNAME;
   var clName = tmpPrefix + "_" + type;
   var cl = readyCL( csName, clName );
   var importFile = tmpFileDir + tmpPrefix + "." + type;
   var importFields = 'a int, b decimal';
   var findCond = { "b": { "$type": 2, "$et": "decimal" } };

   // init import file and expect records
   var recsNum = initImportFile_testPoint1( importFile );
   var expRecs = initExpectData_testPoint1( recsNum );
   // import
   var rc = importData( csName, clName, importFile, type, importFields, true );
   // check results
   checkImportRC( rc, recsNum );
   checkCLData( cl, recsNum, expRecs, findCond );
   // clean data
   cl.truncate();
   cmd.run( "rm -rf " + importFile );

   // init import file and expect records
   var recsNum = initImportFile_testPoint2( importFile );
   var expRecs = initExpectData_testPoint2( recsNum );
   // import
   var rc = importData( csName, clName, importFile, type, importFields, true );
   // check results
   checkImportRC( rc, recsNum );
   checkCLData( cl, recsNum, expRecs, findCond );
   // clean data
   cl.truncate();
   cmd.run( "rm -rf " + importFile );

   // init import file and expect records
   var recsNum = initImportFile_testPoint3( importFile );
   var expRecs = initExpectData_testPoint3( recsNum );
   // import
   var rc = importData( csName, clName, importFile, type, importFields, true );
   // check results
   checkImportRC( rc, recsNum );
   checkCLData( cl, recsNum, expRecs, findCond );
   // clean data
   cl.truncate();
   cmd.run( "rm -rf " + importFile );

   cleanCL( csName, clName );
}

function initImportFile_testPoint1 ( importFile )
{
   var file = fileInit( importFile );
   var recordsNum = 307;
   // 0, b value e.g: ".01E" / ".001E"......
   var str = "";
   var bVal = ".0";
   for( var i = 0; i < recordsNum; i++ )
   {
      str += i + "," + bVal + "1E\n";
      bVal += "0";
   }
   file.write( str );
   file.close();
   return recordsNum;
}

function initImportFile_testPoint2 ( importFile )
{
   var file = fileInit( importFile );
   var recordsNum = 400;
   // 0, b value e.g: ".000000000000001E++0" / ".000000000000001E+1"......".000000000000001E+400"
   var str = "";
   for( var i = 0; i < recordsNum; i++ )
   {
      str += i + ",.000000000000001E+" + i + "\n";
   }
   file.write( str );
   file.close();
   return recordsNum;
}

function initImportFile_testPoint3 ( importFile )
{
   var file = fileInit( importFile );
   var recordsNum = 400;
   // 0, b value e.g: ".01E" / ".011E"......
   var str = "";
   var bVal = "1";
   for( var i = 0; i < recordsNum; i++ )
   {
      str += i + ",.0" + bVal + "E\n";
      bVal += "1";
   }
   file.write( str );
   file.close();
   return recordsNum;
}

function initExpectData_testPoint1 ( expRecsNum )
{
   var expRecs = [];
   var bVal = "0.0";
   for( var i = 0; i < expRecsNum; i++ )
   {
      var record = { "a": i, "b": { "$decimal": bVal + "1" } };
      bVal += "0";
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}

function initExpectData_testPoint2 ( expRecsNum )
{
   var expRecs = [];
   var record;
   var tmpBVal1 = "000000000000001";
   var tmpBVal2 = "1";
   for( var i = 0; i < expRecsNum; i++ )
   {
      if( i < 15 ) // b < 1
      {
         var bVal = "0." + tmpBVal1.substring( i, tmpBVal1.length )
         record = { "a": i, "b": { "$decimal": bVal } };
      }
      else if( i >= 15 )  // 0 > 1
      {
         record = { "a": i, "b": { "$decimal": tmpBVal2 } };
         tmpBVal2 += "0";
      }
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}

function initExpectData_testPoint3 ( expRecsNum )
{
   var expRecs = [];
   var bVal = "0.01";
   for( var i = 0; i < expRecsNum; i++ )
   {
      var record = { "a": i, "b": { "$decimal": bVal } };
      bVal += "1";
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}