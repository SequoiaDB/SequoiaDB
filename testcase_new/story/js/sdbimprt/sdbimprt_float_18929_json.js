/************************************************************************
*@Description:  seqDB-18929: 整数位后n位为0，小数位后n位为0（如10.10） 
*@Author     :  2019-8-6  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18929_json";
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
   var left = "1";
   var id = 1;
   for( var i = 0; i < 20; i++ )
   {
      var right = "1";
      left = left + "0";
      for( var j = 0; j < 20; j++ )
      {
         right = right + "0";
         file.write( '{ "_id": ' + id + ', "a": { "$decimal": "' + left + '.' + right + '" } }\n' );
         ++id;
         file.write( '{ "_id": ' + id + ', "a": ' + left + '.' + right + ' }\n' );
         ++id;
      }
   }
   file.close();
}

function getExpResult ( dataType )
{
   var expResult = [];
   var left = "1";
   for( var i = 0; i < 20; i++ )
   {
      var right = "1";
      left = left + "0";
      for( var j = 0; j < 20; j++ )
      {
         right = right + "0";
         var decimalDate = left + "." + right;
         if( dataType == "decimal" && i < 13 )
         {
            expResult.push( { a: { "$decimal": decimalDate } } );
         }
         else if( dataType == "decimal" && i >= 13 )
         {
            expResult.push( { a: { "$decimal": decimalDate } } );
            expResult.push( { a: { "$decimal": decimalDate } } );
         }
         else if( dataType == "double" && i < 13 )
         {
            expResult.push( { a: parseFloat( left + ".1" ) } );
         }
      }
   }
   return expResult;
}