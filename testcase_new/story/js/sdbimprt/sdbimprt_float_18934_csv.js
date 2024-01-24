/************************************************************************
*@Description:  seqDB-18934: 整数位前n位后m位为0，小数位后x位为0（如010.10）     
*@Author     :  2019-8-6  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18934_csv";
   var csvFile = tmpFileDir + clName + ".csv";

   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( csvFile );

   var fields = "_id int, a int";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 1800 );
   var dataType = "int32";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   var fields = "_id int, a long";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 1800 );
   var dataType = "int64";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   var fields = "_id int, a double";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 1800 );
   dataType = "double";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   var fields = "_id int, a decimal";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 1800 );
   dataType = "decimal";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );

   commDropCL( db, COMMCSNAME, clName );
}

function prepareDate ( typeFile )
{
   var file = new File( typeFile );
   var leftR = "";
   var id = 1;
   for( var i = 0; i < 9; i++ )
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
            right = right + "0";
            file.write( id + "," + left + "." + right + "\n" );
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
   for( var i = 0; i < 9; i++ )
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
            right = right + "0";
            if( dataType == "decimal" )
            {
               expResult.push( { a: { "$decimal": left + "." + right } } );
            }
            else if( dataType == "double" )
            {
               expResult.push( { a: parseFloat( left + ".1" ) } );
            }
            else
            {
               expResult.push( { a: parseInt( left ) } );
            }
         }
      }
   }
   return expResult;
}