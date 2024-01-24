/************************************************************************
*@Description:  seqDB-18930: 整数位后n位为0，小数位前n位后m位为0（如10.010） 
*@Author     :  2019-8-6  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18930_json";
   var jsonFile = tmpFileDir + clName + ".json";

   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( jsonFile );

   var rcResults = importData( COMMCSNAME, clName, jsonFile, "json" );
   checkImportRC( rcResults, 1800 );
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
   for( var i = 0; i < 9; i++ )
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
   var left = "1";
   for( var i = 0; i < 9; i++ )
   {
      var rightL = "";
      left = left + "0";
      for( var j = 0; j < 10; j++ )
      {
         var rightR = "";
         rightL = rightL + "0";
         for( var k = 0; k < 10; k++ )
         {
            //有效位数小于十六位时自动识别为double
            rightR = rightR + "0";
            var decimalDate = left + "." + rightL + "1" + rightR;
            var doubleData = parseFloat( left + "." + rightL + "1" );
            if( dataType == "decimal" && ( i + j ) < 12 )
            {
               expResult.push( { a: { "$decimal": decimalDate } } );
            }
            else if( dataType == "decimal" && ( i + j ) >= 12 )
            {
               expResult.push( { a: { "$decimal": decimalDate } } );
               expResult.push( { a: { "$decimal": decimalDate } } );
            }
            else if( dataType == "double" && ( i + j ) < 12 )
            {
               expResult.push( { a: doubleData } );
            }
         }
      }
   }
   return expResult;
}