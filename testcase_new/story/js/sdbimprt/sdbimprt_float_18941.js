/************************************************************************
*@Description:  seqDB-18941: 整数位和小数位均不为0（如10.11）    
*@Author     :  2019-8-7  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18941";
   var csvFile = tmpFileDir + clName + ".csv";
   var jsonFile = tmpFileDir + clName + ".json";

   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( csvFile );
   prepareDate( jsonFile );

   var fields = "_id int, a";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields );
   checkImportRC( rcResults, 400 );
   dataType = "double";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   dataType = "decimal";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   var fields = "_id int, a";
   var rcResults = importData( COMMCSNAME, clName, jsonFile, "json" );
   checkImportRC( rcResults, 400 );
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
      for( var j = 0; j < 20; j++ )
      {
         right = right + "1";
         var type = typeFile.split( "." ).pop();
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
   for( var i = 0; i < 20; i++ )
   {
      var right = "";
      left = left + "1";
      for( var j = 0; j < 20; j++ )
      {
         right = right + "1";
         //有效数字超过15位自动识别为decimal
         if( dataType == "decimal" && ( i + j ) >= 14 )
         {
            expResult.push( { a: { "$decimal": left + "." + right } } );
         }
         else if( dataType == "double" && ( i + j ) < 14 )
         {
            expResult.push( { a: parseFloat( left + "." + right ) } );
         }
      }
   }
   return expResult;
}