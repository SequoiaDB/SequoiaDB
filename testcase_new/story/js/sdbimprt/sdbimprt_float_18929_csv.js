/************************************************************************
*@Description:  seqDB-18929: 整数位后n位为0，小数位后n位为0（如10.10） 
*@Author     :  2019-8-6  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18929_csv";
   var csvFile = tmpFileDir + clName + ".csv";

   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( csvFile );

   var fields = "_id int, a int";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 180 );
   var dataType = "int32";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   var fields = "_id int, a long";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 180 );
   var dataType = "int64";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   var fields = "_id int, a double";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 180 );
   dataType = "double";
   var expResult = getExpResult( dataType );
   checkResult( cl, dataType, expResult );
   cl.truncate();

   var fields = "_id int, a decimal";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 180 );
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
   //由于实际值为浮点数，整数部分超过十位的时候，强制转换成int32是内存转换，无法确定转换规则，因此控制正数部分最大有效数字为10
   for( var i = 0; i < 9; i++ )
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
   var left = "1";
   for( var i = 0; i < 9; i++ )
   {
      var right = "1";
      left = left + "0";
      for( var j = 0; j < 20; j++ )
      {
         right = right + "0";
         if( dataType == "decimal" )
         {
            var decimalDate = left + "." + right;
            expResult.push( { "a": { "$decimal": decimalDate } } );
         }
         else if( dataType == "double" )
         {
            expResult.push( { "a": parseFloat( left + ".1" ) } );
         }
         else
         {
            expResult.push( { "a": parseInt( left ) } );
         }
      }
   }
   return expResult;
}