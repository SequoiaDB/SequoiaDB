/************************************************************************
*@Description:  seqDB-18936: 整数位前n位后m位为0，小数位全不为0（如10.11） 
*@Author     :  2019-8-6  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18936_json";
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
         var right = "";
         leftL = leftL + "0";
         for( var k = 0; k < 20; k++ )
         {
            right = right + "1";
            var left = leftL + "1" + leftR;
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
         var right = "";
         leftL = leftL + "0";
         for( var k = 0; k < 20; k++ )
         {
            right = right + "1";
            var left = leftL + "1" + leftR;
            if( dataType == "decimal" && ( i + k ) < 13 )
            {
               expResult.push( { a: { "$decimal": "1" + leftR + "." + right } } );
            }
            else if( dataType == "decimal" && ( i + k ) >= 13 )
            {
               expResult.push( { a: { "$decimal": "1" + leftR + "." + right } } );
               expResult.push( { a: { "$decimal": "1" + leftR + "." + right } } );
            }
            else if( dataType == "double" && ( i + k ) < 13 )
            {
               expResult.push( { a: parseFloat( "1" + leftR + "." + right ) } );
            }
         }
      }
   }
   return expResult;
}