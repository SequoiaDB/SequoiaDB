/************************************
*@Description:  seqDB-19125 主表上执行query/limit/sort/skip/hint查询，查询数据在一个子表中
*@author:       luweikang
*@createDate:   2019.9.12
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var mainCLName = "mainCL19125";
   var subCLName = "subcl19125";
   var subCLNum = 2;
   var filePath = WORKDIR + "/subCLLob19125/";
   deleteTmpFile( filePath );
   var scope = 15;
   var beginBound = 20190101;
   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the beginning" );
   var mainCL = createMainCLAndAttachCL( db, COMMCSNAME, mainCLName, subCLName, "YYYYMMDD", subCLNum, beginBound, scope );

   //put lob
   putLobs( mainCL, filePath );

   //test a: listLobs with query[0]; 
   var queryIndex = 0;
   listLobsWithQueryAndCheckResult( mainCL, queryIndex );

   //test a: listLobs with query[500]; 
   var queryIndex = 500;
   listLobsWithQueryAndCheckResult( mainCL, queryIndex );

   //test a: listLobs with query[999]; 
   var queryIndex = 999;
   listLobsWithQueryAndCheckResult( mainCL, queryIndex );

   //test b:listLobs with limit( 0 ); 
   var limitNum = 0;
   listLobsWithLimitAndCheckResult( mainCL, filePath, limitNum );

   //test b:listLobs with limit( 1 ); 
   var limitNum = 1;
   listLobsWithLimitAndCheckResult( mainCL, filePath, limitNum );

   //test b:listLobs with limit( 555 ); query for two subcls
   var limitNum = 555;
   listLobsWithLimitAndCheckResult( mainCL, filePath, limitNum );

   //test b:listLobs with limit( 1000 ); query for three subcls
   var limitNum = 1000;
   listLobsWithLimitAndCheckResult( mainCL, filePath, limitNum );

   //test c :listLobs with sort(), asce Order by Size
   var sortCond = { "Size": 1, "CreateTime": 1 };
   var sortKey = "Size";
   var sortOrder = 1;
   listLobsWithSortAndCheckResult( mainCL, filePath, sortCond, sortKey, sortOrder );

   //test c :listLobs with sort(), desc Order by CreateTime; 
   var sortCond = { "CreateTime": -1, "ModificationTime": -1 };
   var sortKey = "CreateTime";
   var sortOrder = -1;
   listLobsWithSortAndCheckResult( mainCL, filePath, sortCond, sortKey, sortOrder );

   //test d :listLobs with skip()
   var skipNum = 0;
   listLobsWithSkipAndCheckResult( mainCL, filePath, skipNum );

   //test d :listLobs with skip()
   var skipNum = 1000;
   listLobsWithSkipAndCheckResult( mainCL, filePath, skipNum );

   //test d :listLobs with skip()
   var skipNum = 1200;
   listLobsWithSkipAndCheckResult( mainCL, filePath, skipNum );

   //test c: listLobs with skip/limit/sort
   var skipNum = 300;
   var limitNum = 700;
   listLobsWithQueryOptionAndCheckResult( mainCL, filePath, skipNum, limitNum );

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the ending" );
   deleteTmpFile( filePath );
}

function putLobs ( mainCL, filePath )
{
   //lob num for each subcl
   var lobNum = 500;
   var maxLobSize = 1024 * 100;
   var fileName = "lob_" + maxLobSize;

   makeTmpFile( filePath, fileName, maxLobSize );
   for( var i = 0; i < 3; ++i )
   {
      if( i == 0 )
      {
         //put lob in subcl0
         beginDate = "2019-01-01-00.00.00.000000";
      }
      else if( i == 1 )
      {
         //put lob in subcl0
         beginDate = "2019-01-11-00.00.00.000000";
      }
      else
      {
         //put lob in subcl1
         beginDate = "2019-01-21-00.00.00.000000";
      }
      for( var j = 0; j < lobNum; ++j )
      {
         var lobSize = Math.round( Math.random() * maxLobSize );
         var lobOid = mainCL.createLobID( beginDate );
         mainCL.putLob( filePath + fileName, lobOid );
         //truncateLob to construct a different size lob. taks less time than generating files of different sizes.
         mainCL.truncateLob( lobOid, lobSize );
      }
   }
}
