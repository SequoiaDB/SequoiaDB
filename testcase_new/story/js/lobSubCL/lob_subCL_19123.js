/************************************
*@Description:  seqDB-19123 主表上执行listLobs指定属性字段查询
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

   var mainCLName = "mainCL19123";
   var subCLName = "subcl19123";
   var subCLNum = 2;
   var filePath = WORKDIR + "/subCLLob19123/";
   deleteTmpFile( filePath );
   var scope = 5;
   var beginBound = 20191001;
   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the beginning" );
   var mainCL = createMainCLAndAttachCL( db, COMMCSNAME, mainCLName, subCLName, "YYYYMMDD", subCLNum, beginBound, scope );

   //put lob
   var lobSizes = [1024 * 10, 10, 36, 1024 * 10, 1024 * 15, 1024 * 10, 3, 1, 10, 0];
   for( var i = 0; i < lobSizes.length; ++i )
   {
      var fileName = "lob_" + lobSizes[i];
      makeTmpFile( filePath, fileName, lobSizes[i] );
      var beginDate = beginBound + i;
      insertLob( mainCL, filePath + fileName, "YYYYMMDD", scope, 1, 1, beginDate );
   }

   //test a : 指定字段值查询，该字段值存在多条记录
   var attrName = "Size";
   var attrValue = 1024 * 10;
   var matchSymbol = "$et";
   var condition = { "Size": 1024 * 10 };
   listLobsAndCheckResult( mainCL, condition, attrName, attrValue, matchSymbol );

   //test b : 精确匹配一条完整记录查询
   listLobsWithCondition( mainCL );

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the ending" );
   deleteTmpFile( filePath );
}

function listLobsWithCondition ( mainCL )
{
   //精确查找每个lob信息
   var listResult = mainCL.listLobs();
   var expListResult = [];
   while( listResult.next() )
   {
      var listObj = listResult.current().toObj();
      expListResult.push( listObj );
   }

   for( var i = 0; i < expListResult.length; i++ )
   {
      var condition = expListResult[i];
      var actRecs = [];
      var actListResult = mainCL.listLobs( SdbQueryOption().cond( condition ) );
      while( actListResult.next() )
      {
         var listObj = actListResult.current().toObj();
         actRecs.push( listObj );
      }

      if( actRecs.length !== 1 )
      {
         throw new Error( "check lob num", "\nactual value= " + JSON.stringify( actRecs ) + "\ncondition= " + JSON.stringify( condition ) );
      }

      if( JSON.stringify( actRecs[0] ) !== JSON.stringify( condition ) )
      {
         throw new Error( "checkRec()", "rec ERROR, the list condition=" + JSON.stringify( condition ) );
      }

   }
}
