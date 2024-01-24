/************************************************************************
*@Description:  seqDB-18917:整数位和小数位全为0（如00.00） 
*@Author     :  2019-7-30  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18917_json";
   var jsonFile = tmpFileDir + clName + ".json";

   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( jsonFile );

   var rcResults = importData( COMMCSNAME, clName, jsonFile, "json" );
   checkImportRC( rcResults, 840 );
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
   var left = "";
   var num = 0;
   var id = 1;
   for( var i = 0; i < 20; i++ )
   {
      var right = "";
      left = left + "0";
      file.write( '{ "_id": ' + id + ', "a": ' + left + ' }\n' );
      ++id;
      file.write( '{ "_id": ' + id + ', "a": { "$numberLong": "' + left + '' + '" } }\n' );
      ++id;
      for( var j = 0; j < 20; j++ )
      {
         right = right + "0";
         file.write( '{ "_id": ' + id + ', "a": { "$decimal": "' + left + '.' + right + '" } }\n' );
         ++id;
         file.write( '{ "_id": ' + id + ', "a": ' + left + '.' + right + ' }\n' );
         ++id;
      }
   }
   file.close()
}

function getExpResult ( dataType )
{
   var expResult = [];
   if( dataType == "decimal" )
   {
      for( var i = 0; i < 20; i++ )
      {
         var decimalDate = "0.";
         for( var j = 0; j < 20; j++ )
         {
            decimalDate = decimalDate + "0";
            expResult.push( { a: { "$decimal": decimalDate } } );
         }
      }
   }
   else if( dataType == "double" )
   {
      for( var i = 0; i < 400; i++ )
      {
         expResult.push( { a: 0 } );
      }
   }
   else
   {
      for( var i = 0; i < 20; i++ )
      {
         expResult.push( { a: 0 } );
      }
   }
   return expResult;
}