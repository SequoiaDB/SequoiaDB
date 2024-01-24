/************************************
*@Description:  seqDB-19114 主表上listLobs使用gt/gte/lt/lte/ne/et匹配查询，匹配数据跨子表
*@author:      wuyan
*@createDate:  2019.8.21
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var mainCLName = "mainCL19114";
   var subCLName = "subcl19114";
   var subCLNum = 2;
   var filePath = WORKDIR + "/subCLLob19114/";
   deleteTmpFile( filePath );
   var scope = 5;
   var beginBound = 20190901;
   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the beginning" );
   var mainCL = createMainCLAndAttachCL( db, COMMCSNAME, mainCLName, subCLName, "YYYYMMDD", subCLNum, beginBound, scope );

   //put lob
   var lobSizes = [1024, 10, 36, 1024 * 10, 1024 * 15, 1024 * 20, 3, 1, 10, 0];
   for( var i = 0; i < lobSizes.length; ++i )
   {
      var fileName = "lob_" + lobSizes[i];
      makeTmpFile( filePath, fileName, lobSizes[i] );
      var beginDate = beginBound + i;
      insertLob( mainCL, filePath + fileName, "YYYYMMDD", scope, 1, 1, beginDate );
   }

   //listLobs use $lte, match field is "Size"
   var attrName = "Size";
   var attrValue = 1024 * 15;
   var matchSymbol = "$lte";
   var condition = { "Size": { "$lte": 1024 * 15 } };
   listLobsAndCheckResult( mainCL, condition, attrName, attrValue, matchSymbol );

   //listLobs use $gte, match field is "Size"
   var attrValue = 10;
   var matchSymbol = "$gte";
   var condition = { "Size": { "$gte": 10 } };
   listLobsAndCheckResult( mainCL, condition, attrName, attrValue, matchSymbol );

   //listLobs use $lt, match field is "Size"
   var attrValue = 1024 * 20;
   var matchSymbol = "$lt";
   var condition = { "Size": { "$lt": attrValue } };
   listLobsAndCheckResult( mainCL, condition, attrName, attrValue, matchSymbol );

   //listLobs use $gt, match field is "Size"
   var attrValue = 36;
   var matchSymbol = "$gt";
   var condition = { "Size": { "$gt": attrValue } };
   listLobsAndCheckResult( mainCL, condition, attrName, attrValue, matchSymbol );

   //listLobs use $et, match field is "Size"
   var attrValue = 10;
   var matchSymbol = "$et";
   var condition = { "Size": { "$et": attrValue } };
   listLobsAndCheckResult( mainCL, condition, attrName, attrValue, matchSymbol );

   //listLobs use $ne, match field is "Available"
   var attrName = "Available";
   var attrValue = false;
   var matchSymbol = "$ne";
   var condition = { "Available": { "$ne": attrValue } };
   listLobsAndCheckResult( mainCL, condition, attrName, attrValue, matchSymbol );

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the ending" );
   deleteTmpFile( filePath );
}
