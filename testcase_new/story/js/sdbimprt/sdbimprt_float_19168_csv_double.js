/************************************************************************
*@Description:  seqDB-19168:科学计数法，底数为整数（如 1E+308）
*@Author     :  2019-8-21  huangxiaoni
************************************************************************/

main( test );

function test ()
{
   var type = 'csv';
   var tmpPrefix = "sdbimprt_double_19168";
   var csName = COMMCSNAME;
   var clName = tmpPrefix + "_" + type;
   var cl = readyCL( csName, clName );
   var findCond = { "b": { "$type": 2, "$et": "double" } };
   var importFile = tmpFileDir + tmpPrefix + "." + type;
   var importFields = 'a int, b double';

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
   // test point2, b value e.g: 1E+308, 11E+307 ...... xxxE+0
   var recordsNum;
   var str = "";
   var aVal = 0;
   var bValBaseNum = 1;
   var bValIndexNum = 307;
   recordsNum = bValIndexNum + 2;
   while( bValIndexNum >= 0 )
   {
      str += aVal + "," + bValBaseNum + "E+" + bValIndexNum + "\n";
      bValBaseNum += "1";
      bValIndexNum--;
      aVal++;
   }
   str += "308,1.7976111111111111e+308";
   var file = fileInit( importFile );
   file.write( str );
   file.close();
   return recordsNum;
}

function initExpectData_testPoint1 ( expRecsNum )
{
   var expRecs = [];
   var record = {};
   var tmpBVal = 1;
   for( var i = 0; i < expRecsNum; i++ ) 
   {
      if( i <= 20 )
      {
         record = { "a": i, "b": tmpBVal };
         tmpBVal = tmpBVal * 10;
      }
      else if( i > 20 && i < 93 ) 
      {
         record = { "a": i, "b": Number( "1e+" + i ) };
      }
      else if( i === 93 ) 
      {
         record = { "a": i, "b": Number( "9.999999999999999e+" + ( i - 1 ) ) };
      }
      else if( i > 93 && i < 279 ) 
      {
         record = { "a": i, "b": Number( "1e+" + i ) };
      }
      else if( i === 279 ) 
      {
         record = { "a": i, "b": Number( "9.999999999999999e+" + ( i - 1 ) ) };
      }
      else if( i > 279 && i < 284 ) 
      {
         record = { "a": i, "b": Number( "1e+" + i ) };
      }
      else if( i === 284 )
      {
         record = { "a": i, "b": Number( "9.999999999999999e+" + ( i - 1 ) ) };
      }
      else if( i > 284 && i < 309 )
      {
         record = { "a": i, "b": Number( "1e+" + i ) };
      }
      else 
      {
         record = { "a": i, "b": Infinity };
      }
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}

function initExpectData_testPoint2 ( expRecsNum )
{
   var expRecs = [];
   var record = {};
   var tmpBVal = "1.1";
   for( var i = 0; i < expRecsNum; i++ ) 
   {
      if( i === 0 ) 
      {
         record = { "a": i, "b": 1e+307 };
      }
      else if( i > 0 && i < expRecsNum - 1 )
      {
         record = { "a": i, "b": Number( tmpBVal + "e+307" ) };
         if( i < 15 ) 
         {
            tmpBVal += "1";
         }
      }
      else
      {
         record = { "a": i, "b": 1.797611111111111e+308 };
      }
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}