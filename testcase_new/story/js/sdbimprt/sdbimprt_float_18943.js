/************************************************************************
*@Description:  seqDB-18943: 底数的整数位和小数位全为0，指数为309（如00.00e+309） 
*@Author     :  2019-8-8  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18943";
   var csvFile = tmpFileDir + clName + ".csv";
   var jsonFile = tmpFileDir + clName + ".json";

   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( csvFile );
   prepareDate( jsonFile );

   var fields = "_id int, a int";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 80 );
   dataType = "int32";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   var fields = "_id int, a long";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 80 );
   dataType = "int64";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   var fields = "_id int, a double";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 80 );
   dataType = "double";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   var fields = "_id int, a decimal";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 80 );
   dataType = "decimal";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   var fields = "_id int, a";
   var rcResults = importData( COMMCSNAME, clName, jsonFile, "json" );
   checkImportRC( rcResults, 80 );
   dataType = "double";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );

   commDropCL( db, COMMCSNAME, clName );
}

function prepareDate ( typeFile )
{
   var file = new File( typeFile );
   var left = "";
   var right = "";
   var id = 1;
   for( var i = 0; i < 20; i++ )
   {
      left = left + "0";
      right = right + "0";
      var type = typeFile.split( "." ).pop();
      if( type == "csv" )
      {
         file.write( id + "," + left + "." + right + "e+308" + "\n" );
         ++id;
         file.write( id + "," + left + "." + right + "e-308" + "\n" );
         ++id;
         file.write( id + "," + left + "." + right + "e+309" + "\n" );
         ++id;
         file.write( id + "," + left + "." + right + "e-309" + "\n" );
         ++id;
      }
      else
      {
         file.write( '{ "_id": ' + id + ', "a":' + left + '.' + right + "e+308" + ' }\n' );
         ++id;
         file.write( '{ "_id": ' + id + ', "a":' + left + '.' + right + "e-308" + ' }\n' );
         ++id;
         file.write( '{ "_id": ' + id + ', "a":' + left + '.' + right + "e+309" + ' }\n' );
         ++id;
         file.write( '{ "_id": ' + id + ', "a":' + left + '.' + right + "e-309" + ' }\n' );
         ++id;
      }
   }
   file.close();
}

function getExpResult ( dataType )
{
   var expResult = [];
   var right = "";
   for( var i = 0; i < 308; i++ )
   {
      right = right + 0;
   }

   for( var i = 0; i < 20; i++ )
   {
      right = right + 0;
      if( dataType === "decimal" )
      {
         expResult.push( { a: { "$decimal": "0" } } );
         expResult.push( { a: { "$decimal": "0." + right } } );
         expResult.push( { a: { "$decimal": "0" } } );
         expResult.push( { a: { "$decimal": "0.0" + right } } );
      }
      else
      {
         for( var j = 0; j < 4; j++ )
         {
            expResult.push( { a: 0 } );
         }
      }
   }
   return expResult;
}
