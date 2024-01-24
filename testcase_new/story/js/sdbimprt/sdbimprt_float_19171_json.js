/************************************************************************
*@Description:  seqDB-19171:科学计数法，底数为整数+小数点，且整数位全为0（000.E+309）
*@Author     :  2019-8-21  huangxiaoni
************************************************************************/

main( test );

function test ()
{
   var type = 'json';
   var tmpPrefix = "sdbimprt_19171";
   var csName = COMMCSNAME;
   var clName = tmpPrefix + "_" + type;
   var cl = readyCL( csName, clName );
   var importFile = tmpFileDir + tmpPrefix + "." + type;

   // init import file and expect records
   var recsNum = initImportFile_testPoint( importFile );
   // import
   var rc = importData( csName, clName, importFile, type );
   // check results
   checkImportRC( rc, recsNum );
   var findTypeArr = ["int32", "int64", "double", "decimal"];
   for( var i = 0; i < findTypeArr.length; i++ )
   {
      var expRecs = initExpectData_testPoint( recsNum, findTypeArr[i] );
      var findCond = { "b": { "$type": 2, "$et": findTypeArr[i] } };
      var expRecsNum = JSON.parse( expRecs ).length;
      checkCLData( cl, expRecsNum, expRecs, findCond );
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
      str += "{a:" + i + ",b:" + bVal + "}\n";
      bVal = "0" + bVal;
   }

   // 400, b value e.g: "0.E+0" / "0.E+1"......"0.E+400"
   for( var i = tmpNum; i < tmpNum * 2; i++ )
   {
      str += "{a:" + i + ",b:0.E+" + ( i - tmpNum ) + "}\n";
   }

   file.write( str );
   file.close();
   return recordsNum;
}

function initExpectData_testPoint ( expRecsNum, findType )
{
   var expRecs = [];
   if( findType === "double" ) 
   {
      for( var i = 0; i < expRecsNum; i++ )
      {
         var record = { "a": i, "b": 0 };
         expRecs.push( JSON.stringify( record ) );
      }
   }
   return "[" + expRecs + "]";
}