/************************************************************************
*@Description:  seqDB-19172:科学计数法，底数为小数点+小数位，小数位包含有效数字（.001E+310）
         import type: long
*@Author     :  2019-8-21  huangxiaoni
************************************************************************/
main( test );

function test ()
{
   var type = 'csv';
   var tmpPrefix = "sdbimprt_long_19172";
   var csName = COMMCSNAME;
   var clName = tmpPrefix + "_" + type;
   var cl = readyCL( csName, clName );
   var importFile = tmpFileDir + tmpPrefix + "." + type;
   var importFields = 'a int, b long';
   var findCond = { "b": { "$type": 2, "$et": "int64" } };

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

   if ( commIsArmArchitecture() == false )
   {
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
   }
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
   for( var i = 0; i < expRecsNum; i++ )
   {
      var record = { "a": i, "b": 0 };
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}

function initExpectData_testPoint2 ( expRecsNum )
{
   var expRecs = [];
   var record;
   for( var i = 0; i < expRecsNum; i++ )
   {
      if( i < 15 || i >= 324 ) // b < 0 || b > xE+307
      {
         record = { "a": i, "b": 0 };
      }
      else if( i >= 15 && i < 31 )  // 0 < b < 9223372036854775807
      {
         var bVal = 1 * Math.pow( 10, ( i - 15 ) );
         record = { "a": i, "b": bVal };
      }
      else if( i >= 31 && i < 34 )  // 0 < b < 9223372036854775807
      {
         var bVal = 1 * Math.pow( 10, ( i - 15 ) );
         record = { "a": i, "b": { "$numberLong": bVal + "" } };
      }
      else if( i >= 34 && i < 324 )  // b > 9223372036854775807
      {
         record = { "a": i, "b": { "$numberLong": "-9223372036854775808" } };
      }
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}

function initExpectData_testPoint3 ( expRecsNum )
{
   var expRecs = [];
   for( var i = 0; i < expRecsNum; i++ )
   {
      var record = { "a": i, "b": 0 };
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}