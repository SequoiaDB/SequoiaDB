/************************************************************************
*@Description:  seqDB-18918: 整数位全为0，小数位前n位为0（如00.01） 
*@Author     :  2019-7-31  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18918_csv";
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
      var right = "";
      left = left + "0";
      for( var j = 0; j < 20; j++ )
      {
         file.write( id + "," + left + "." + right + "1\n" );
         ++id;
         right = right + "0";
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
         for( var j = 0; j < 20; j++ )
         {
            var decimalDate = "0." + rightL + "1";
            rightL = rightL + "0";
            expResult.push( { a: { "$decimal": decimalDate } } );
         }
      }
   }
   else if( dataType == "double" )
   {
      for( var i = 0; i < 20; i++ )
      {
         expResult.push( { a: 0.1 } );
         expResult.push( { a: 0.01 } );
         expResult.push( { a: 0.001 } );
         expResult.push( { a: 0.0001 } );
         expResult.push( { a: 0.00001 } );
         expResult.push( { a: 0.000001 } );
         expResult.push( { a: ( 1e-7 ) } );
         expResult.push( { a: ( 1e-8 ) } );
         expResult.push( { a: ( 1e-9 ) } );
         expResult.push( { a: ( 1e-10 ) } );
         expResult.push( { a: ( 1e-11 ) } );
         expResult.push( { a: ( 1e-12 ) } );
         expResult.push( { a: ( 1e-13 ) } );
         expResult.push( { a: ( 1e-14 ) } );
         expResult.push( { a: ( 1e-15 ) } );
         expResult.push( { a: ( 1e-16 ) } );
         expResult.push( { a: ( 1e-17 ) } );
         expResult.push( { a: ( 1e-18 ) } );
         expResult.push( { a: ( 1e-19 ) } );
         expResult.push( { a: ( 1e-20 ) } );
      }
   }
   else
   {
      for( var i = 0; i < 400; i++ )
      {
         expResult.push( { a: 0 } );
      }
   }
   return expResult;
}