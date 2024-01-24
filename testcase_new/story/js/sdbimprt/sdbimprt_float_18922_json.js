/************************************************************************
*@Description:  seqDB-18922: 整数位前n位为0，小数位全为0（如01.00）  
*@Author     :  2019-8-1  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18922_json";
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
   var left = "1";
   var id = 1;
   for( var i = 0; i < 20; i++ )
   {
      var right = "";
      left = "0" + left;
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
   file.close();
}

function getExpResult ( dataType )
{
   var expResult = [];
   if( dataType == "int32" || dataType == "int64" )
   {
      for( var i = 0; i < 20; i++ )
      {
         expResult.push( { a: 1 } );
      }
   }
   else
   {
      for( var i = 0; i < 20; i++ )
      {
         var decimalDate = "1.";
         for( var j = 0; j < 20; j++ )
         {
            if( dataType == "decimal" )
            {
               decimalDate = decimalDate + "0";
               expResult.push( { a: { "$decimal": decimalDate } } );
            }
            else if( dataType == "double" )
            {
               expResult.push( { a: 1 } );
            }
         }
      }
   }
   return expResult;
}