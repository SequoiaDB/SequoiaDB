/************************************************************************
*@Description:  seqDB-19174:浮点数特殊值测试（如0. / .0 / . / .E / 00）
*@Author     :  2019-8-21  huangxiaoni
************************************************************************/

main( test );

function test ()
{
   var type = 'csv';
   var tmpPrefix = "sdbimprt_19174";
   var csName = COMMCSNAME;
   var clName = tmpPrefix + "_" + type;
   var cl = readyCL( csName, clName );
   var importFile = tmpFileDir + tmpPrefix + "." + type;

   // init import file and expect records
   var recsNum = initImportFile_testPoint( importFile );
   var expRecsNum = recsNum - 2;
   // import and check results, cover type: int, long, double, decimal
   var bValTypeArr = ["int", "long", "double", "decimal"];
   var findTypeArr = ["int32", "int64", "double", "decimal"];
   for( var i = 0; i < bValTypeArr.length; i++ )
   {
      var expRecs = initExpectData_testPoint( expRecsNum, bValTypeArr[i] );
      // import
      var importFields = 'a int, b ' + bValTypeArr[i];
      var rc = importData( csName, clName, importFile, type, importFields, true );
      // check results
      var expParseFailureNum = 2;
      checkImportRC( rc, expRecsNum, expRecsNum, expParseFailureNum );
      var findCond = { "b": { "$type": 2, "$et": findTypeArr[i] } };
      checkCLData( cl, expRecsNum, expRecs, findCond );
      cl.truncate();
   }
   // clean data
   cleanCL( csName, clName );
   cmd.run( "rm -rf " + importFile );
   var tmpRec = csName + "_" + clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );
}

function initImportFile_testPoint ( importFile )
{
   var file = fileInit( importFile );
   var tmpNum = 400;
   var recordsNum = tmpNum * 3 + 2;

   // 0, b value e.g: "0." / "00."......
   var str = "";
   var bVal = "0.";
   for( var i = 0; i < tmpNum; i++ )
   {
      str += i + "," + bVal + "\n";
      bVal = "0" + bVal;
   }

   // 400, b value e.g: ".0" / ".00"......
   var bVal = ".0";
   for( var i = tmpNum; i < tmpNum * 2; i++ )
   {
      str += i + "," + bVal + "\n";
      bVal += "0";
   }

   // 800, b value e.g: "0" / "00"......
   var bVal = "0";
   for( var i = tmpNum * 2; i < tmpNum * 3; i++ )
   {
      str += i + "," + bVal + "\n";
      bVal += "0";
   }

   // 1201, b value e.g: "."
   str += ( tmpNum * 3 ) + ",.\n";
   str += ( tmpNum * 3 + 1 ) + ",.E\n";

   file.write( str );
   file.close();
   return recordsNum;
}

function initExpectData_testPoint ( expRecsNum, bValType )
{
   var expRecs = [];
   var bVal = "0.0";
   for( var i = 0; i < expRecsNum; i++ )
   {
      var record = { "a": i, "b": 0 };
      if( bValType === "decimal" )
      {
         record = { "a": i, "b": { "$decimal": "0" } };
         if( i >= 400 && i < 800 ) 
         {
            record = { "a": i, "b": { "$decimal": bVal } };
            bVal += "0";
         }
      }
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}