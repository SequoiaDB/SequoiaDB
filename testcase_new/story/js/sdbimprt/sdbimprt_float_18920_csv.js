/************************************************************************
*@Description:  seqDB-18920: 整数位全为0，小数位前n位后m位为0（如00.010）  
*@Author     :  2019-8-1  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18920_csv";
   var csvFile = tmpFileDir + clName + ".csv";

   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( csvFile );

   var fields = "_id int, a int";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 2000 );
   var dataType = "int32";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   var fields = "_id int, a long";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 2000 );
   var dataType = "int64";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   var fields = "_id int, a double";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 2000 );
   dataType = "double";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   var fields = "_id int, a decimal";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 2000 );
   dataType = "decimal";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );

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
   if( dataType == "double" )
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
   else
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
               if( dataType == "decimal" )
               {
                  var decimalData = "0." + right;
                  expResult.push( { a: { "$decimal": decimalData } } );
               }
               else
               {
                  expResult.push( { a: 0 } );
               }
            }
         }
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