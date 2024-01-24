/************************************************************************
*@Description:  seqDB-18919: 整数位全为0，小数位后n位为0（如00.10） 
*@Author     :  2019-7-31  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18919_json";
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
   if( dataType == "decimal" )
   {
      for( var i = 0; i < 20; i++ )
      {
         var rightR = "";
         for( var j = 0; j < 20; j++ )
         {
            rightR = rightR + "0";
            var decimalDate = "0.1" + rightR;
            expResult.push( { a: { "$decimal": decimalDate } } );
         }
      }
   }
   else
   {
      for( var i = 0; i < 400; i++ )
      {
         expResult.push( { a: 0.1 } );
      }
   }
   return expResult;
}