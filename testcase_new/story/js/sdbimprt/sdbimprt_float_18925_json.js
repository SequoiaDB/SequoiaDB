/************************************************************************
*@Description:  seqDB-18925: 整数位前n位为0，小数位前n位后m位为0（如01.010）   
*@Author     :  2019-8-2  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18925_json";
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
   var left = "1";
   var right = "";
   var id = 1;
   for( var i = 0; i < 20; i++ )
   {
      var rightL = "";
      left = "0" + left;
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
   var left = "";
   for( var i = 0; i < 20; i++ )
   {
      var rightL = "";
      left = "0" + left;
      for( var j = 0; j < 10; j++ )
      {
         var rightR = "";
         rightL = rightL + "0";
         for( var k = 0; k < 10; k++ )
         {
            rightR = rightR + "0";
            if( dataType == "decimal" )
            {
               var decimalDate = "1." + rightL + "1" + rightR;
               expResult.push( { a: { "$decimal": decimalDate } } );
            }
            else if( dataType == "double" )
            {
               var doubleData = parseFloat( "1." + rightL + "1" );
               expResult.push( { a: doubleData } );
            }
         }
      }
   }
   return expResult;
}