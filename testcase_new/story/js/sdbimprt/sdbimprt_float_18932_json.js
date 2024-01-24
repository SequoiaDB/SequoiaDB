/************************************************************************
*@Description:  seqDB-18932: 整数位前n位后m位为0，小数位全为0（如010.00）   
*@Author     :  2019-8-6  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18932_json";
   var jsonFile = tmpFileDir + clName + ".json";

   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( jsonFile );

   var rcResults = importData( COMMCSNAME, clName, jsonFile, "json" );
   checkImportRC( rcResults, 3780 );
   var expResult = getExpResult( "int32" );
   checkResult( cl, "int32", expResult );
   var expResult = getExpResult( "int64" );
   checkResult( cl, "int64", expResult );
   var expResult = getExpResult( "double" );
   checkResult( cl, "double", expResult );
   var expResult = getExpResult( "decimal" );
   checkResult( cl, "decimal", expResult );

   commDropCL( db, COMMCSNAME, clName );
}

function prepareDate ( typeFile )
{
   var file = new File( typeFile );
   var leftR = "";
   var id = 1;
   for( var i = 0; i < 9; i++ )
   {
      var leftL = "";
      leftR = leftR + "0";
      for( var j = 0; j < 10; j++ )
      {
         var right = "";
         leftL = leftL + "0";
         var left = leftL + "1" + leftR;
         file.write( '{ "_id": ' + id + ', "a": ' + left + ' }\n' );
         ++id;
         file.write( '{ "_id": ' + id + ', "a": { "$numberLong": "' + left + '' + '" } }\n' );
         ++id;
         for( var k = 0; k < 20; k++ )
         {
            right = right + "0";
            file.write( '{ "_id": ' + id + ', "a": ' + left + '.' + right + ' }\n' );
            ++id;
            file.write( '{ "_id": ' + id + ', "a": { "$decimal": "' + left + '.' + right + '" } }\n' );
            ++id;
         }
      }
   }
   file.close();
}

function getExpResult ( dataType )
{
   var expResult = [];
   var leftR = "";
   for( var i = 0; i < 9; i++ )
   {
      var leftL = "";
      leftR = leftR + "0";
      for( var j = 0; j < 10; j++ )
      {
         var right = "";
         leftL = leftL + "0";
         var left = "1" + leftR;
         if( dataType == "int32" || dataType == "int64" )
         {
            expResult.push( { a: parseFloat( left ) } );
         }
         else 
         {
            for( var k = 0; k < 20; k++ )
            {
               right = right + "0";
               if( dataType == "decimal" )
               {
                  expResult.push( { a: { "$decimal": left + "." + right } } );
               }
               else
               {
                  expResult.push( { a: parseFloat( left ) } );
               }
            }
         }
      }
   }
   return expResult;
}