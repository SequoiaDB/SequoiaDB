/************************************
*@Description:  seqDB-19121 普通表上listLobs使用include/default/multiply/divide匹配查询
*@author:       luweikang
*@createDate:   2019.9.12
**************************************/
main( test );

function test ()
{
   var clName = "cl19121";
   var filePath = WORKDIR + "/CLLob19121/";
   deleteTmpFile( filePath );
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   //put lob
   var lobSizes = [1024, 10, 36, 1024 * 10, 1024 * 95, 1024 * 20, 3, 1, 10, 0, 1024 * 1024 * 8, 1024 * 2, 36, 1024 * 30, 1024 * 55, 1024 * 80, 1024 * 1024 * 3, 98, 80, 2];
   for( var i = 0; i < lobSizes.length; ++i )
   {
      var fileName = "lob_" + lobSizes[i];
      makeTmpFile( filePath, fileName, lobSizes[i] );
      insertLob( cl, filePath + fileName, "YYYYMMDD", 1, 1, 1 );
   }

   //listLobs use $include
   var selSymbol = "$include";
   var condition = { "Size": { "$lte": 1024 * 1024 * 3 } };
   var selCondition = { "Size": { "$include": 1 } };
   listLobsWithSelCondAndCheckResult( cl, selSymbol, selCondition, condition );

   //listLobs use $multiply
   var modifyValue = 10;
   var selSymbol = "$multiply";
   var condition = { "Size": { "$lte": 1024 * 1024 * 20 } };
   var selCondition = { "Size": { "$multiply": modifyValue } };
   listLobsWithSelCondAndCheckResult( cl, selSymbol, selCondition, condition, modifyValue );

   //listLobs use $divide
   var modifyValue = 2;
   var selSymbol = "$divide";
   var condition = { "Size": { "$gt": 36 } };
   var selCondition = { "Size": { "$divide": modifyValue } };
   listLobsWithSelCondAndCheckResult( cl, selSymbol, selCondition, condition, modifyValue );

   //listLobs use $default
   var selSymbol = "$default";
   var condition = { "Size": { "$gt": 0 } };
   var selCondition = { "Size": { "$default": 1024 } };
   listLobsWithSelCondAndCheckResult( cl, selSymbol, selCondition, condition );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
   deleteTmpFile( filePath );
}
