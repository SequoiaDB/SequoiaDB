/************************************************************************
*@Description:  seqDB-18923: 整数位前n位为0，小数位前n位为0（如01.00）  
*@Author     :  2019-8-2  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18923_json";
   var jsonFile = tmpFileDir + clName + ".json";

   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( jsonFile );

   var rcResults = importData( COMMCSNAME, clName, jsonFile, "json" );
   checkImportRC( rcResults, 800 );
   var expResult = getExpResult( "double" );
   checkResult( cl, "double", expResult );
   var expResult = getExpResult( "decimal" );
   checkResult( cl, "decimal", expResult );

   commDropCL( db, COMMCSNAME, clName );
}

function prepareDate ( typeFile )
{
   var file = new File( typeFile );
   var right = "1";
   var id = 1;
   for( var i = 0; i < 20; i++ )
   {
      var left = "1";
      right = "0" + right;
      for( var j = 0; j < 20; j++ )
      {
         left = "0" + left;
         file.write( '{ "_id": ' + id + ', "a": { "$decimal": "' + left + '.' + right + '" } }\n' );
         ++id;
         file.write( '{ "_id": ' + id + ', "a": ' + left + '.' + right + ', b: "double" }\n' );
         ++id;
      }
   }
   file.close();
}

function getExpResult ( dataType )
{
   var expResult = [];
   var right = "1";
   for( var i = 0; i < 20; i++ )
   {
      right = "0" + right;
      for( var j = 0; j < 20; j++ )
      {
         var decimalData = "1." + right;
         var doubleData = parseFloat( "1." + right );
         if( dataType == "decimal" && i < 13 )
         {
            expResult.push( { a: { "$decimal": decimalData } } );
         }
         else if( dataType == "decimal" && i >= 13 )
         {
            expResult.push( { a: { "$decimal": decimalData } } );
            expResult.push( { a: { "$decimal": decimalData }, b: "double" } );
         }
         else if( dataType == "double" && i < 13 )
         {
            expResult.push( { a: doubleData, b: "double" } );
         }
      }
   }
   return expResult;
}