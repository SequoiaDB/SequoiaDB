/************************************************************************
*@Description:  seqDB-18933: 整数位前n位后m位为0，小数位前x位为0（如010.01）    
*@Author     :  2019-8-6  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18933_json";
   var jsonFile = tmpFileDir + clName + ".json";

   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( jsonFile );

   var rcResults = importData( COMMCSNAME, clName, jsonFile, "json" );
   checkImportRC( rcResults, 4000 );
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
   for( var i = 0; i < 10; i++ )
   {
      var leftL = "";
      leftR = leftR + "0";
      for( var j = 0; j < 10; j++ )
      {
         var right = "1";
         leftL = leftL + "0";
         for( var k = 0; k < 20; k++ )
         {
            left = leftL + "1" + leftR;
            right = "0" + right;
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
   for( var i = 0; i < 10; i++ )
   {
      var leftL = "";
      leftR = leftR + "0";
      for( var j = 0; j < 10; j++ )
      {
         var right = "1";
         leftL = leftL + "0";
         for( var k = 0; k < 20; k++ )
         {
            left = "1" + leftR;
            right = "0" + right;
            if( dataType == "decimal" && ( i + k ) < 12 )
            {
               expResult.push( { a: { "$decimal": left + "." + right } } );
            }
            else if( dataType == "decimal" && ( i + k ) >= 12 )
            {
               expResult.push( { a: { "$decimal": left + "." + right } } );
               expResult.push( { a: { "$decimal": left + "." + right } } );
            }
            else if( dataType == "double" && ( i + k ) < 12 )
            {
               expResult.push( { a: parseFloat( left + "." + right ) } );
            }
         }
      }
   }
   return expResult;
}