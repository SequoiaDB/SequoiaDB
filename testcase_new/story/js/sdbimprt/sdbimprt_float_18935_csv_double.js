/************************************************************************
*@Description:  seqDB-18935: 整数位前n位后m位为0，小数位前x位后y位为0（如10.010）     
*@Author     :  2019-8-6  zhaoxiaoni
************************************************************************/

main( test );

function test ()
{
   var clName = "cl_18935_csv_double";
   var csvFile = tmpFileDir + clName + ".csv";

   var cl = commCreateCL( db, COMMCSNAME, clName );
   prepareDate( csvFile );

   var fields = "a int, b double";
   var rcResults = importData( COMMCSNAME, clName, csvFile, "csv", fields, true );
   checkImportRC( rcResults, 80 );
   var cond = { "b": { "$type": 2, "$et": "double" } };
   var expResult = getExpResult();
   checkCLData( cl, 80, expResult, cond );

   commDropCL( db, COMMCSNAME, clName );
}

function prepareDate ( typeFile )
{
   var file = new File( typeFile );
   var left = "10";
   var right = "01000000000000000000";
   for( var i = 0; i < 20; i++ )
   {
      left = "0" + left;
      file.write( i + ", " + left + "." + right + "\n" );
   }

   left = "01";
   right = "00000000000000000010";
   for( var i = 20; i < 40; i++ )
   {
      left = left + "0";
      file.write( i + ", " + left + "." + right + "\n" );
   }

   left = "00000000000000000010";
   right = "01";
   for( var i = 40; i < 60; i++ )
   {
      right = right + "0";
      file.write( i + ", " + left + "." + right + "\n" );
   }

   left = "010000000000000000000";
   right = "10";
   for( var i = 60; i < 80; i++ )
   {
      right = right + "0";
      file.write( i + ", " + left + "." + right + "\n" );
   }

   file.close();
}

function getExpResult ()
{
   var expResult = [];
   for( var i = 0; i < 20; i++ )
   {
      expResult.push( { a: i, b: 10.01 } );
   }

   var left = "1";
   for( var i = 20; i < 40; i++ )
   {
      left = left + "0";
      expResult.push( { a: i, b: parseFloat( left ) } );
   }

   for( var i = 40; i < 60; i++ )
   {
      expResult.push( { a: i, b: 10.01 } );
   }

   left = "10000000000000000000";
   for( var i = 60; i < 80; i++ )
   {
      expResult.push( { a: i, b: parseFloat( left ) } );
   }

   return JSON.stringify( expResult );
}