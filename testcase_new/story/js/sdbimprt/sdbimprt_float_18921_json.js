/************************************************************************
*@Description:  seqDB-18921: 整数位全为0，小数位不为0（如00.11）   
*@Author     :  2019-8-1  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18921_json";
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
   var left = "";
   var id = 1;
   for( var i = 0; i < 20; i++ )
   {
      var right = "";
      left = left + "0";
      for( var j = 0; j < 20; j++ )
      {
         right = right + "1";
         file.write( '{ "_id": ' + id + ', "a": ' + left + '.' + right + ' }\n' );
         ++id;
         file.write( '{ "_id": ' + id + ', "a": { "$decimal": "' + left + '.' + right + '" } }\n' );
         ++id;
      }
   }
   file.close();
}

function getExpResult ( dataType )
{
   var expResult = [];
   for( var i = 0; i < 20; i++ )
   {
      var right = "";
      for( var j = 0; j < 20; j++ )
      {
         right = right + "1";
         var decimalData = "0." + right;
         if( j < 15 && dataType == "decimal" )
         {
            expResult.push( { a: { "$decimal": decimalData } } );
         }
         else if( j >= 15 && dataType == "decimal" )
         {
            expResult.push( { a: { "$decimal": decimalData } } );
            expResult.push( { a: { "$decimal": decimalData } } );
         }
         else if( j < 15 && dataType == "double" )
         {
            expResult.push( { a: parseFloat( parseFloat( "0." + right ).toFixed( 16 ) ) } );
         }
      }
   }
   return expResult;
}