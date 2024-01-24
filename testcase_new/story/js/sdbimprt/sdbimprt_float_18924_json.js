/************************************************************************
*@Description:  seqDB-18924: 整数位前n位为0，小数位后n位为0（如01.10）   
*@Author     :  2019-8-2  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18924_json";
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
   var right = "1";
   var id = 1;
   for( var i = 0; i < 20; i++ )
   {
      var left = "1";
      right = right + "0";
      for( var j = 0; j < 20; j++ )
      {
         left = "0" + left;
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
   var right = "1";
   for( var i = 0; i < 20; i++ )
   {
      right = right + "0";
      for( var j = 0; j < 20; j++ )
      {
         if( dataType == "decimal" )
         {
            var decimalDate = "1." + right;
            expResult.push( { a: { "$decimal": decimalDate } } );
         }
         else
         {
            expResult.push( { a: 1.1 } );
         }
      }
   }
   return expResult;
}