/************************************
*@Description: seqDB-19116 普通表上listLobs使用gt/gte/lt/lte/ne/et匹配查询
*@author:      luweikang
*@createDate:  2019.9.11
**************************************/
main( test );

function test ()
{
   var clName = "cl19116";
   var filePath = WORKDIR + "/CLLob19116/";
   deleteTmpFile( filePath );
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   //put lob
   var lobSizes = [1024, 10, 36, 1024 * 10, 1024 * 15, 1024 * 20, 3, 1, 10, 0];
   for( var i = 0; i < lobSizes.length; ++i )
   {
      var fileName = "lob_" + lobSizes[i];
      makeTmpFile( filePath, fileName, lobSizes[i] );
      insertLob( cl, filePath + fileName, "YYYYMMDD", 1, 1, 1 );
   }

   //listLobs use $lt
   var attrName = "Size";
   var attrValue = 3;
   var matchSymbol = "$lt";
   var condition = { "Size": { "$lt": 3 } };
   listLobsAndCheckResult( cl, condition, attrName, attrValue, matchSymbol );

   //listLobs use $lte
   var attrValue = 3;
   var matchSymbol = "$lte";
   var condition = { "Size": { "$lte": 3 } };
   listLobsAndCheckResult( cl, condition, attrName, attrValue, matchSymbol );

   //listLobs use $gte
   var attrValue = 1024 * 11;
   var matchSymbol = "$gte";
   var condition = { "Size": { "$gte": 1024 * 11 } };
   listLobsAndCheckResult( cl, condition, attrName, attrValue, matchSymbol );

   //listLobs use $gt
   var attrValue = 1024 * 10;
   var matchSymbol = "$gt";
   var condition = { "Size": { "$gt": 1024 * 10 } };
   listLobsAndCheckResult( cl, condition, attrName, attrValue, matchSymbol );

   //listLobs use $et
   var attrValue = 1024 * 10;
   var matchSymbol = "$et";
   var condition = { "Size": { "$et": 1024 * 10 } };
   listLobsAndCheckResult( cl, condition, attrName, attrValue, matchSymbol );

   //listLobs use $ne
   var attrName = "Available";
   var attrValue = true;
   var matchSymbol = "$ne";
   var condition = { "Available": { "$ne": true } };
   listLobsAndCheckResult( cl, condition, attrName, attrValue, matchSymbol );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
   deleteTmpFile( filePath );
}
