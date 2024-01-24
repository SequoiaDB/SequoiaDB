/************************************************************************
*@Description:  seqDB-18919: 整数位全为0，小数位后n位为0（如00.10） 
*@Author     :  2019-7-31  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18919_csv";
   var csvFile = tmpFileDir + clName + ".csv";

   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( csvFile );

   var fields = "_id int, a int";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 400 );
   var dataType = "int32";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   var fields = "_id int, a long";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 400 );
   var dataType = "int64";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   var fields = "_id int, a double";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 400 );
   dataType = "double";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   var fields = "_id int, a decimal";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 400 );
   dataType = "decimal";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );

   commDropCL( db, COMMCSNAME, clName );
}

function prepareDate ( typeFile )
{
   var file = new File( typeFile );
   var left = "";
   var id = 1;
   for( var i = 0; i < 20; i++ )
   {
      var right = "1";
      left = left + "0";
      for( var j = 0; j < 20; j++ )
      {
         right = right + "0";
         file.write( id + "," + left + "." + right + "\n" );
         ++id;
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
         var rightR = "";
         for( var j = 0; j < 20; j++ )
         {
            rightR = rightR + "0";
            var decimalDate = "0.1" + rightR;
            expResult.push( { a: { "$decimal": decimalDate } } );
         }
      }
   }
   else
   {
      for( var i = 0; i < 400; i++ )
      {
         if( dataType == "double" )
         {
            expResult.push( { a: 0.1 } );
         }
         else
         {
            expResult.push( { a: 0 } );
         }
      }
   }
   return expResult;
}