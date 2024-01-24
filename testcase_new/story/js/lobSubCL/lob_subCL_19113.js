/************************************
*@Description:  seqDB-19113 主表上listLobs使用gt/gte/lt/lte/ne/et匹配查询，匹配数据在一个子表中
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

   var mainCLName = "mainCL19113";
   var subCLName = "subcl19113";
   var subCLNum = 2;
   var filePath = WORKDIR + "/subCLLob19113/";
   deleteTmpFile( filePath );
   var scope = 5;
   var beginBound = 20190801;
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

   //listLobs use $lt
   var attrName = "Size";
   var attrValue = 3;
   var matchSymbol = "$lt";
   var condition = { "Size": { "$lt": 3 } };
   listLobsAndCheckResult( mainCL, condition, attrName, attrValue, matchSymbol );

   //listLobs use $lte
   var attrValue = 3;
   var matchSymbol = "$lte";
   var condition = { "Size": { "$lte": 3 } };
   listLobsAndCheckResult( mainCL, condition, attrName, attrValue, matchSymbol );

   //listLobs use $gte
   var attrValue = 1024 * 11;
   var matchSymbol = "$gte";
   var condition = { "Size": { "$gte": 1024 * 11 } };
   listLobsAndCheckResult( mainCL, condition, attrName, attrValue, matchSymbol );

   //listLobs use $gt
   var attrValue = 1024 * 10;
   var matchSymbol = "$gt";
   var condition = { "Size": { "$gt": 1024 * 10 } };
   listLobsAndCheckResult( mainCL, condition, attrName, attrValue, matchSymbol );

   //listLobs use $et
   var attrValue = 1024 * 10;
   var matchSymbol = "$et";
   var condition = { "Size": { "$et": 1024 * 10 } };
   listLobsAndCheckResult( mainCL, condition, attrName, attrValue, matchSymbol );

   //listLobs use $ne
   var attrName = "Available";
   var attrValue = true;
   var matchSymbol = "$ne";
   var condition = { "Available": { "$ne": true } };
   listLobsAndCheckResult( mainCL, condition, attrName, attrValue, matchSymbol );

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the ending" );
   deleteTmpFile( filePath );
}
