/************************************************************************
*@Description:  seqDB-18945:底数的整数位和小数位前n位后m位为0，有效整数位+指数=308位（如01.010e+308） 
*@Author     :  2019-8-8  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18945";
   var csvFile = tmpFileDir + clName + ".csv";
   var jsonFile = tmpFileDir + clName + ".json";

   var cl = commCreateCL( db, COMMCSNAME, clName );
   var expResult = prepareDate( csvFile );

   var fields = "a";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields );
   checkImportRC( rcResults, 2000 );
   dataType = "double";
   checkResult( cl, dataType, expResult );
   cl.truncate();

   expResult = prepareDate( jsonFile );
   var fields = "a";
   var rcResults = importData( COMMCSNAME, clName, jsonFile, "json" );
   checkImportRC( rcResults, 2000 );
   dataType = "double";
   checkResult( cl, dataType, expResult );

   commDropCL( db, COMMCSNAME, clName );
}

function prepareDate ( typeFile )
{
   var file = new File( typeFile );
   var left = "";
   var right = "";
   var expResult = [];
   for( var i = 0; i < 20; i++ )
   {
      var rightL = "1";
      left = left + "0";
      for( var j = 0; j < 10; j++ )
      {
         var rightR = "";
         rightL = "0" + rightL;
         for( var k = 0; k < 10; k++ )
         {
            rightR = rightR + "0";
            right = rightL + rightR;
            var type = typeFile.split( "." ).pop();
            if( type == "csv" )
            {
               file.write( left + "." + right + "e+" + ( 310 + j ) + "\n" );
            }
            else 
            {
               file.write( '{ a:' + left + '.' + right + "e+" + ( 310 + j ) + ' }\n' );
            }
            expResult.push( { a: parseFloat( left + "." + right + "e+" + ( 310 + j ) ) } );
         }
      }
   }
   file.close();
   return expResult;
}