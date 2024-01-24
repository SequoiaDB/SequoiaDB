/************************************************************************
*@Description:  seqDB-18927:整数位后n位为0，小数位全为0（如10.00）
*@Author     :  2019-8-6  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18927_json";
   var jsonFile = tmpFileDir + clName + ".json";

   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( jsonFile );

   var rcResults = importData( COMMCSNAME, clName, jsonFile, "json" );
   checkImportRC( rcResults, 420 );
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
   var left = "1";
   var id = 1;
   for( var i = 0; i < 20; i++ )
   {
      var right = "";
      left = left + "0";
      file.write( '{ "_id": ' + id + ', "a": ' + left + ' }\n' );
      ++id;
      for( var j = 0; j < 20; j++ )
      {
         right = right + "0";
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
   if( dataType == "int64" )
   {
      expResult.push( { a: 10000000000 } );
      expResult.push( { a: 100000000000 } );
      expResult.push( { a: 1000000000000 } );
      expResult.push( { a: 10000000000000 } );
      expResult.push( { a: 100000000000000 } );
      expResult.push( { a: 1000000000000000 } );
      expResult.push( { a: { "$numberLong": "10000000000000000" } } );
      expResult.push( { a: { "$numberLong": "100000000000000000" } } );
      expResult.push( { a: { "$numberLong": "1000000000000000000" } } );
   }
   for( var i = 0; i < 20; i++ )
   {
      var right = "";
      left = left + "0";
      if( dataType == "int32" && i < 9 )
      {
         expResult.push( { a: parseInt( left ) } );
      } else if( dataType == "decimal" && i >= 18 )
      {
         expResult.push( { a: { "$decimal": left } } );
      }
      for( var j = 0; j < 20; j++ )
      {
         right = right + "0";
         if( dataType == "decimal" )
         {
            if( i >= 14 && i < 18 )
            {
               expResult.push( { a: { "$decimal": left + "." + right } } );
            }
            else if( i >= 18 )
            {
               expResult.push( { a: { "$decimal": left + "." + right } } );
            }
         }
         else if( dataType == "double" && i < 14 )
         {
            expResult.push( { a: parseFloat( left ) } );
         }
      }
   }
   return expResult;
}