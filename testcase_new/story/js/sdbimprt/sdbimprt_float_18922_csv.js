/************************************************************************
*@Description:  seqDB-18922: 整数位前n位为0，小数位全为0（如01.00）  
*@Author     :  2019-8-1  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18922_csv";
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
   var right = "";
   var id = 1;
   for( var i = 0; i < 20; i++ )
   {
      var left = "1";
      right = right + "0";
      for( var j = 0; j < 20; j++ )
      {
         left = "0" + left;
         file.write( id + "," + left + "." + right + "\n" );
         ++id;
      }
   }
   file.close();
}

function getExpResult ( dataType )
{
   var expResult = [];
   var right = "";
   for( var i = 0; i < 20; i++ )
   {
      right = right + "0";
      for( var j = 0; j < 20; j++ )
      {
         var decimalDate = "1." + right;
         if( dataType == "decimal" )
         {
            expResult.push( { "a": { "$decimal": decimalDate } } );
         }
         else
         {
            expResult.push( { "a": 1 } );
         }
      }
   }
   return expResult;
}