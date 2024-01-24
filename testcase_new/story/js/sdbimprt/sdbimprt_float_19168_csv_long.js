/************************************************************************
*@Description:  seqDB-19168:科学计数法，底数为整数（如 1E+308）
*@Author     :  2019-8-21  huangxiaoni
************************************************************************/
main( test );

function test ()
{
   var type = 'csv';
   var tmpPrefix = "sdbimprt_long_19168";
   var csName = COMMCSNAME;
   var clName = tmpPrefix + "_" + type;
   var cl = readyCL( csName, clName );
   var findCond = { "b": { "$type": 2, "$et": "int64" } };
   var importFile = tmpFileDir + tmpPrefix + "." + type;
   var importFields = 'a int, b long';

   if ( commIsArmArchitecture() == false )
   {
      // test point1, b value e.g: 1E+19, 1E+9 ...... 1E+0
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
   }
   
   // test poin3, bValIndexNum > 308, b value e.g: 1E+309
   // init import file and expect records
   var recsNum = initImportFile_testPoint3( importFile );
   var expRecs = initExpectData_testPoint3( recsNum );
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
   // test point1, b value e.g: 1E+19, 1E+9 ...... 1E+0
   var recordsNum;
   var str = "";
   var aVal = 0;
   var bValBaseNum = 1;
   var bValIndexNum = 19; //b value < 9223372036854775807
   recordsNum = bValIndexNum + 1;
   while( bValIndexNum >= 0 )
   {
      str += aVal + "," + bValBaseNum + "E+" + bValIndexNum + "\n";
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
   var bValIndexNum = 308; //b value < 2147483648
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

function initImportFile_testPoint3 ( importFile )
{
   // test point3, b value e.g: bValIndexNum > 308, b value e.g: 1E+309
   var recordsNum = 1;
   var aVal = 0;
   var bValBaseNum = 1;
   var bValIndexNum = 309;
   var str = aVal + "," + bValBaseNum + "E+" + bValIndexNum + "\n";
   var file = fileInit( importFile );
   file.write( str );
   file.close();
   return recordsNum;
}

function initExpectData_testPoint1 ( expRecsNum )
{
   var expRecs = [];
   var record = {};
   var tmpBVal = 1E+18;
   for( i = 0; i < expRecsNum; i++ ) 
   {
      if( i === 0 ) 
      {
         record = { "a": i, "b": { $numberLong: "-9223372036854775808" } };
      }
      else if( i > 0 && i < 4 )
      {
         record = { "a": i, "b": { $numberLong: String( tmpBVal ) } };
         tmpBVal = tmpBVal / 10;
      }
      else
      {
         record = { "a": i, "b": tmpBVal };
         tmpBVal = tmpBVal / 10;
      }
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}

function initExpectData_testPoint2 ( expRecsNum )
{
   var expRecs = [];
   var record = {};
   for( i = 0; i < expRecsNum; i++ ) 
   {
      if( i < 19 ) 
      {
         record = { "a": i, "b": { $numberLong: "-9223372036854775808" } };
      }
      else
      {
         record = { "a": i, "b": 0 };
      }
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}

function initExpectData_testPoint3 ( expRecsNum )
{
   var expRecs = [];
   var record = { a: 0, b: 0 };
   expRecs.push( JSON.stringify( record ) );
   return "[" + expRecs + "]";
}