/************************************************************************
*@Description:  seqDB-19170:科学计数法，底数为整数+小数点，整数包含有效数字（如1.E+308） 
         import type: long
*@Author     :  2019-8-21  huangxiaoni
************************************************************************/
main( test );

function test ()
{
   var type = 'csv';
   var tmpPrefix = "sdbimprt_long_19170";
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

   cleanCL( csName, clName );
}

function initImportFile_testPoint1 ( importFile )
{
   var file = fileInit( importFile );
   var recordsNum = 400;
   // 0, b value e.g: "1.E" / "11.E"......
   var str = "";
   var bVal = "1.E";
   for( var i = 0; i < recordsNum; i++ )
   {
      str += i + "," + bVal + "\n";
      bVal = "1" + bVal;
   }
   file.write( str );
   file.close();
   return recordsNum;
}

function initImportFile_testPoint2 ( importFile )
{
   var file = fileInit( importFile );
   var recordsNum = 400;
   // 0, b value e.g: "1.E+0" / "1.E+1"......"1.E+400"
   var str = "";
   for( var i = 0; i < recordsNum; i++ )
   {
      str += i + ",1.E+" + i + "\n";
   }
   file.write( str );
   file.close();
   return recordsNum;
}

function initExpectData_testPoint1 ( expRecsNum )
{
   var expRecs = [];
   var tmpBVal = "1";
   var records;
   for( var i = 0; i < expRecsNum; i++ )
   {
      if( i <= 15 )
      {
         record = { "a": i, "b": Number( tmpBVal ) };
         tmpBVal += "1";
      }
      else if( i > 15 && i <= 18 )
      {
         record = { "a": i, "b": { "$numberLong": tmpBVal } };
         tmpBVal += "1";
      }
      else
      {
         record = { "a": i, "b": 0 };
      }
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
      if( i <= 15 ) 
      {
         var bVal = 1 * Math.pow( 10, i );
         record = { "a": i, "b": bVal };
      }
      else if( i > 15 && i < 19 )  // 0 < b < 9223372036854775807
      {
         var bVal = 1 * Math.pow( 10, i );
         record = { "a": i, "b": { "$numberLong": bVal + "" } };
      }
      else if( i >= 19 && i <= 308 )  // b < 1e+308
      {
         record = { "a": i, "b": { "$numberLong": "-9223372036854775808" } };
      }
      else if( i > 308 ) 
      {
         var record = { "a": i, "b": 0 };
      }
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}