/************************************************************************
*@Description:  seqDB-19170:科学计数法，底数为整数+小数点，整数包含有效数字（如1.E+308） 
         import type: decimal
*@Author     :  2019-8-21  huangxiaoni
************************************************************************/


main( test );

function test ()
{
   var type = 'csv';
   var tmpPrefix = "sdbimprt_decimal_19170";
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
   var bVal = "1";
   for( var i = 0; i < expRecsNum; i++ )
   {
      var record = { "a": i, "b": { "$decimal": bVal } };
      bVal += "1";
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}

function initExpectData_testPoint2 ( expRecsNum )
{
   var expRecs = [];
   var bVal = "1";
   for( var i = 0; i < expRecsNum; i++ )
   {
      var record = { "a": i, "b": { "$decimal": bVal } };
      bVal += "0";
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}