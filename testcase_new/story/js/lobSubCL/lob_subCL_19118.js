/************************************
*@Description:  seqDB-19118 主表上listLobs使用include/default/add/subtract匹配查询，匹配数据在一个子表中
*@author:      wuyan
*@createDate:  2019.8.23
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var mainCLName = "mainCL19118";
   var subCLName = "subcl19118";
   var subCLNum = 2;
   var filePath = WORKDIR + "/subCLLob19118/";
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

   //listLobs use $include
   var selSymbol = "$include";
   var condition = { "Size": { "$gte": 1024 * 11 } };
   var selCondition = { "Size": { "$include": 1 } };
   listLobsWithSelCondAndCheckResult( mainCL, selSymbol, selCondition, condition, modifyValue );

   //listLobs use $add
   var modifyValue = 1024;
   var selSymbol = "$add";
   var condition = { "Size": { "$gt": 1024 * 10 } };
   var selCondition = { "Size": { "$add": modifyValue } };
   listLobsWithSelCondAndCheckResult( mainCL, selSymbol, selCondition, condition, modifyValue );

   //listLobs use $subtract
   var modifyValue = 1;
   var selSymbol = "$subtract";
   var condition = { "Size": { "$lt": 36 } };
   var selCondition = { "Size": { "$subtract": modifyValue } };
   listLobsWithSelCondAndCheckResult( mainCL, selSymbol, selCondition, condition, modifyValue );

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the ending" );
   deleteTmpFile( filePath );
}
