/************************************************************************
*@Description:  seqDB-18920: 整数位全为0，小数位前n位后m位为0（如00.010）  
*@Author     :  2019-8-1  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18920_json";
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
   var left = "";
   var right = "";
   var id = 1;
   for( var i = 0; i < 20; i++ )
   {
      var rightL = "";
      left = left + "0";
      for( var j = 0; j < 10; j++ )
      {
         var rightR = "";
         rightL = rightL + "0";
         for( var k = 0; k < 10; k++ )
         {
            rightR = rightR + "0";
            right = rightL + "1" + rightR;
            file.write( '{ "_id": ' + id + ', "a": { "$decimal": "' + left + '.' + right + '" } }\n' );
            ++id;
            file.write( '{ "_id": ' + id + ', "a": ' + left + '.' + right + ' }\n' );
            ++id;
         }
      }
   }
   file.close();
}

function getExpResult ( dataType )
{
   var expResult = [];
   if( dataType == "decimal" )
   {
      for( var i = 0; i < 20; i++ )
      {
         var rightL = "";
         for( var j = 0; j < 10; j++ )
         {
            var rightR = "";
            rightL = rightL + "0";
            for( var k = 0; k < 10; k++ )
            {
               rightR = rightR + "0";
               right = rightL + "1" + rightR;
               var decimalData = "0." + right;
               expResult.push( { a: { "$decimal": decimalData } } );
            }
         }
      }
   }
   else
   {
      for( var i = 0; i < 20; i++ )
      {
         executeFor( expResult, { a: 0.01 } );
         executeFor( expResult, { a: 0.001 } );
         executeFor( expResult, { a: 0.0001 } );
         executeFor( expResult, { a: 0.00001 } );
         executeFor( expResult, { a: 0.000001 } );
         executeFor( expResult, { a: ( 1e-7 ) } );
         executeFor( expResult, { a: ( 1e-8 ) } );
         executeFor( expResult, { a: ( 1e-9 ) } );
         executeFor( expResult, { a: ( 1e-10 ) } );
         executeFor( expResult, { a: ( 1e-11 ) } );
      }
   }
   return expResult;
}
function executeFor ( expResult, data )
{
   for( var i = 0; i < 10; i++ )
   {
      expResult.push( data );
   }
   return expResult;
}