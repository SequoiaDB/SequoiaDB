/************************************************************************
*@Description:  seqDB-19170:科学计数法，底数为整数+小数点，整数包含有效数字（如1.E+308） 
*@Author     :  2019-8-21  huangxiaoni
************************************************************************/


main( test );

function test ()
{
   var type = 'json';
   var tmpPrefix = "sdbimprt_19170";
   var csName = COMMCSNAME;
   var clName = tmpPrefix + "_" + type;
   var cl = readyCL( csName, clName );
   var importFile = tmpFileDir + tmpPrefix + "." + type;

   // init import file and expect records
   var recsNum = initImportFile_testPoint1( importFile );
   // import
   var rc = importData( csName, clName, importFile, type );
   // check results
   checkImportRC( rc, recsNum );
   var findTypeArr = ["int32", "int64", "double", "decimal"];
   for( var i = 0; i < findTypeArr.length; i++ )
   {
      var expRecs = initExpectData_testPoint1( recsNum, findTypeArr[i] );
      var findCond = { "b": { "$type": 2, "$et": findTypeArr[i] } };
      var expRecsNum = JSON.parse( expRecs ).length;
      checkCLData( cl, expRecsNum, expRecs, findCond );
   }
   // clean data
   cmd.run( "rm -rf " + importFile );
   cl.truncate();

   // init import file and expect records
   var recsNum = initImportFile_testPoint2( importFile );
   // import
   var rc = importData( csName, clName, importFile, type );
   // check results
   checkImportRC( rc, recsNum );
   var findTypeArr = ["int32", "int64", "double", "decimal"];
   for( var i = 0; i < findTypeArr.length; i++ )
   {
      var findCond = { "b": { "$type": 2, "$et": findTypeArr[i] } };
      var expRecs = initExpectData_testPoint2( recsNum, findTypeArr[i] );
      var expRecsNum = JSON.parse( expRecs ).length;
      checkCLData( cl, expRecsNum, expRecs, findCond );
   }
   // clean data
   cmd.run( "rm -rf " + importFile );
   cl.truncate();

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
      str += "{a:" + i + ",b:" + bVal + "+0}\n";
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
      str += "{a:" + i + ",b:1.E+" + i + "}\n";
   }
   file.write( str );
   file.close();
   return recordsNum;
}

function initExpectData_testPoint1 ( expRecsNum, findType )
{
   var expRecs = [];
   var record;
   var bVal = "1";
   for( var i = 0; i < expRecsNum; i++ )
   {
      if( findType === "double" )
      {
         if( i < 15 )
         {
            record = { "a": i, "b": Number( bVal ) };
            bVal += "1";
            expRecs.push( JSON.stringify( record ) );
         }
      }
      else if( findType === "decimal" )
      {
         if( i >= 15 && i < 400 )
         {
            record = { "a": i, "b": { "$decimal": "111111111111111" + bVal } };
            bVal += "1";
            expRecs.push( JSON.stringify( record ) );
         }
      }
   }
   return "[" + expRecs + "]";
}

function initExpectData_testPoint2 ( expRecsNum, findType )
{
   var expRecs = [];
   var record;
   var tmpBVal = "1";
   for( var i = 0; i < expRecsNum; i++ )
   {
      if( findType === "double" && i < 309 ) 
      {
         if( i < 21 ) 
         {
            var bVal = 1 * Math.pow( 10, i );
            record = { "a": i, "b": bVal };
         }
         else if( i >= 21 && i <= 308 )  // b < 1e+308
         {
            if( i === 93 )
            {
               var bVal = 9.999999999999999e+92;
               record = { "a": i, "b": bVal };
            }
            else if( i === 279 )
            {
               var bVal = 9.999999999999999e+278;
               record = { "a": i, "b": bVal };
            }
            else if( i === 284 )
            {
               var bVal = 9.999999999999999e+283;
               record = { "a": i, "b": bVal };
            }
            else
            {
               var bVal = "1e+" + i;
               record = { "a": i, "b": Number( bVal ) };
            }
         }
         expRecs.push( JSON.stringify( record ) );
      }
      else if( findType === "decimal" && i >= 309 ) 
      {
         record = { "a": i, "b": { "$decimal": tmpBVal } };
         expRecs.push( JSON.stringify( record ) );
      }
      tmpBVal += "0";
   }
   return "[" + expRecs + "]";
}