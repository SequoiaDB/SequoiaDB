/************************************************************************
*@Description:  seqDB-19171:科学计数法，底数为整数+小数点，且整数位全为0（000.E+309）
*@Author     :  2019-8-21  huangxiaoni
************************************************************************/

main( test );

function test ()
{
   var type = 'csv';
   var tmpPrefix = "sdbimprt_19171";
   var csName = COMMCSNAME;
   var clName = tmpPrefix + "_" + type;
   var cl = readyCL( csName, clName );
   var importFile = tmpFileDir + tmpPrefix + "." + type;

   // init import file and expect records
   var recsNum = initImportFile_testPoint( importFile );
   // import and check results, cover type: int, long, double, decimal
   var bValTypeArr = ["int", "long", "double", "decimal"];
   var findTypeArr = ["int32", "int64", "double", "decimal"];
   for( var i = 0; i < bValTypeArr.length; i++ )
   {
      // import
      var importFields = 'a int, b ' + bValTypeArr[i];
      var rc = importData( csName, clName, importFile, type, importFields, true );
      // check results
      checkImportRC( rc, recsNum );
      var expRecs = initExpectData_testPoint( recsNum, bValTypeArr[i] );
      var findCond = { "b": { "$type": 2, "$et": findTypeArr[i] } };
      checkCLData( cl, recsNum, expRecs, findCond );
      cl.truncate();
   }

   // clean data
   cmd.run( "rm -rf " + importFile );
   cleanCL( csName, clName );
}

function initImportFile_testPoint ( importFile )
{
   var file = fileInit( importFile );
   var tmpNum = 400;
   var recordsNum = tmpNum * 2;

   // 0, b value e.g: "0.E" / "00.E"......
   var str = "";
   var bVal = "0.E";
   for( var i = 0; i < tmpNum; i++ )
   {
      str += i + "," + bVal + "\n";
      bVal = "0" + bVal;
   }

   // 400, b value e.g: "0.E+0" / "0.E+1"......"0.E+400"
   for( var i = tmpNum; i < tmpNum * 2; i++ )
   {
      str += i + ",0.E+" + ( i - tmpNum ) + "\n";
   }

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
      }
      expRecs.push( JSON.stringify( record ) );
   }
   return "[" + expRecs + "]";
}