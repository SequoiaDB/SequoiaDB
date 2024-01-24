/************************************
*@Description:  seqDB-19122 存在和lob元数据相同的记录，主表上listLobs指定相同字段查询
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

   var mainCLName = "mainCL19122";
   var subCLName = "subcl19122";
   var subCLNum = 1;
   var filePath = WORKDIR + "/subCLLob19122/";
   deleteTmpFile( filePath );
   var scope = 5;
   var beginDate = "20190102";
   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the beginning" );
   var mainCL = createMainCLAndAttachCL( db, COMMCSNAME, mainCLName, subCLName, "YYYYMM", subCLNum, beginDate, scope );

   //put lob
   var fileName = "lob_19222";
   var lobSize = 1024 * 20;
   var lobNum = 5;
   makeTmpFile( filePath, fileName, lobSize );
   insertLob( mainCL, filePath + fileName, "YYYYMM", scope, lobNum, subCLNum, beginDate );

   listLobsWithCondition( mainCL, lobSize, beginDate );

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the ending" );
   deleteTmpFile( filePath );
}

function listLobsWithCondition ( mainCL, lobSize, beginDate )
{
   var listResult = mainCL.listLobs( SdbQueryOption().sort( { "Oid": 1 } ) );
   var expListResult = [];
   while( listResult.next() )
   {
      var listObj = listResult.current().toObj();
      expListResult.push( listObj );
      //插入相同值的记录
      var insertRecord = listResult.current().toObj();
      insertRecord["date"] = beginDate;
      mainCL.insert( insertRecord );
   }

   var condition = { "Size": lobSize };
   var actRecs = [];
   var actListResult = mainCL.listLobs( SdbQueryOption().cond( condition ).sort( { "Oid": 1 } ) );
   while( actListResult.next() )
   {
      var listObj = actListResult.current().toObj();
      actRecs.push( listObj );
   }

   if( JSON.stringify( actRecs ) !== JSON.stringify( expListResult ) )
   {
      throw new Error( "checkRec()", "\nactual value= " + JSON.stringify( actRecs ) + "\nexpect value= " + JSON.stringify( expListResult ) );
   }

}
