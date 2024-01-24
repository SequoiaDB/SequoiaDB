/************************************************************************
*@Description:  seqDB-18926: 整数位前n位为0，小数位全不为0（如01.11）    
*@Author     :  2019-8-2  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18926_csv";
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
   var left = "1";
   var id = 1;
   for( var i = 0; i < 20; i++ )
   {
      var right = "";
      left = "0" + left;
      for( var j = 0; j < 20; j++ )
      {
         right = right + "1";
         file.write( id + "," + left + "." + right + "\n" );
         ++id;
      }
   }
   file.close();
}

function getExpResult ( dataType )
{
   var expResult = [];
   for( var i = 0; i < 20; i++ )
   {
      var right = "";
      for( var j = 0; j < 20; j++ )
      {
         right = right + "1";
         if( dataType == "decimal" )
         {
            var decimalDate = "1." + right;
            expResult.push( { "a": { "$decimal": decimalDate } } );
         }
         else if( dataType == "double" )
         {
            var doubleData = parseFloat( parseFloat( "1." + right ).toFixed( 15 ) );
            expResult.push( { "a": doubleData } );
         }
         else
         {
            expResult.push( { "a": 1 } );
         }
      }
   }
   return expResult;
}
