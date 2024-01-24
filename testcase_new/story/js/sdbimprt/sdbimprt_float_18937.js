/************************************************************************
*@Description:  seqDB-18937: 整数位前n位后m位为0，小数位全不为0（如10.11） 
*@Author     :  2019-8-6  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18937";
   var csvFile = tmpFileDir + clName + ".csv";
   var jsonFile = tmpFileDir + clName + ".json";

   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( csvFile );
   prepareDate( jsonFile );

   var fields = "_id int, a";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields );
   checkImportRC( rcResults, 420 );
   dataType = "int32";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   dataType = "int64";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   dataType = "double";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   dataType = "decimal";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   var fields = "_id int, a";
   var rcResults = importData( COMMCSNAME, clName, jsonFile, "json" );
   checkImportRC( rcResults, 420 );
   dataType = "int32";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   dataType = "int64";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   dataType = "double";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   dataType = "decimal";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );

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
      left = left + "1";
      var type = typeFile.split( "." ).pop();
      if( type == "csv" )
      {
         file.write( id + "," + left + "\n" );
      }
      else
      {
         file.write( '{ "_id": ' + id + ', "a":' + left + ' }\n' );
      }
      ++id;
      for( var j = 0; j < 20; j++ )
      {
         right = right + "0";
         if( type == "csv" )
         {
            file.write( id + "," + left + "." + right + "\n" );
         }
         else
         {
            file.write( '{ "_id": ' + id + ', "a":' + left + '.' + right + ' }\n' );
         }
         ++id;
      }
   }
   file.close();
}

function getExpResult ( dataType )
{
   var expResult = [];
   var left = "";
   if( dataType == "int64" )
   {
      expResult.push( { a: 11111111111 } );
      expResult.push( { a: 111111111111 } );
      expResult.push( { a: 1111111111111 } );
      expResult.push( { a: 11111111111111 } );
      expResult.push( { a: 111111111111111 } );
      expResult.push( { a: 1111111111111111 } );
      expResult.push( { a: { "$numberLong": "11111111111111111" } } );
      expResult.push( { a: { "$numberLong": "111111111111111111" } } );
      expResult.push( { a: { "$numberLong": "1111111111111111111" } } );
   }
   for( var i = 0; i < 20; i++ )
   {
      var right = "";
      left = left + "1";
      if( dataType == "int32" && i < 10 )
      {
         expResult.push( { a: parseInt( left ) } );
      } else if( dataType == "decimal" && i == 19 )
      {
         expResult.push( { a: { "$decimal": left } } );
      }
      for( var j = 0; j < 20; j++ )
      {
         right = right + "0";
         if( dataType == "decimal" && i >= 15 && i < 19 )
         {
            expResult.push( { a: { "$decimal": left + "." + right } } );
         }
         else if( dataType == "decimal" && i == 19 )
         {
            expResult.push( { a: { "$decimal": left + "." + right } } );
         }
         else if( dataType == "double" && i < 15 )
         {
            expResult.push( { a: parseFloat( left ) } );
         }
      }
   }
   return expResult;
}