/************************************************************************
*@Description:  seqDB-18931: 整数位后n位为0，小数位全不为0（如10.11）  
*@Author     :  2019-8-6  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18931_json";
   var jsonFile = tmpFileDir + clName + ".json";

   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( jsonFile );

   var rcResults = importData( COMMCSNAME, clName, jsonFile, "json" );
   checkImportRC( rcResults, 400 );
   var expResult = getExpResult( "double" );
   checkResult( cl, "double", expResult );
   var expResult = getExpResult( "decimal" );
   checkResult( cl, "decimal", expResult );

   commDropCL( db, COMMCSNAME, clName );
}

function prepareDate ( typeFile )
{
   var file = new File( typeFile );
   var left = "1";
   var id = 1;
   for( var i = 0; i < 10; i++ )
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
   var left = "1";
   for( var i = 0; i < 10; i++ )
   {
      var right = "";
      left = left + "0";
      for( var j = 0; j < 20; j++ )
      {
         right = right + "1";
         var decimalDate = left + "." + right;
         if( dataType == "decimal" && ( i + j ) < 13 )
         {
            expResult.push( { a: { "$decimal": decimalDate } } );
         }
         else if( dataType == "decimal" && ( i + j ) >= 13 )
         {
            expResult.push( { a: { "$decimal": decimalDate } } );
            expResult.push( { a: { "$decimal": decimalDate } } );
         }
         else if( dataType == "double" && ( i + j ) < 13 )
         {
            var doubleData = parseFloat( left + "." + right );
            expResult.push( { a: doubleData } );
         }
      }
   }
   return expResult;
}