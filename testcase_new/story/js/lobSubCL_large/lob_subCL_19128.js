/************************************
*@Description:  seqDB-19128 普通表上执行query/limit/sort/skip/hint查询
*@author:       luweikang
*@createDate:   2019.9.12
**************************************/
main( test );

function test ()
{
   var clName = "cl19128";
   var filePath = WORKDIR + "/CLLob19128/";
   deleteTmpFile( filePath );
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );
   var cl = commCreateCL( db, COMMCSNAME, clName );

   //put lob
   putLobs( cl, filePath );

   //test a: listLobs with query[0]; 
   var queryIndex = 0;
   listLobsWithQueryAndCheckResult( cl, queryIndex );

   //test a: listLobs with query[500]; 
   var queryIndex = 500;
   listLobsWithQueryAndCheckResult( cl, queryIndex );

   //test a: listLobs with query[999]; 
   var queryIndex = 999;
   listLobsWithQueryAndCheckResult( cl, queryIndex );

   //test b:listLobs with limit( 0 ); 
   var limitNum = 0;
   listLobsWithLimitAndCheckResult( cl, filePath, limitNum );

   //test b:listLobs with limit( 1 ); 
   var limitNum = 1;
   listLobsWithLimitAndCheckResult( cl, filePath, limitNum );

   //test b:listLobs with limit( 555 ); query for two subcls
   var limitNum = 555;
   listLobsWithLimitAndCheckResult( cl, filePath, limitNum );

   //test b:listLobs with limit( 1000 ); query for three subcls
   var limitNum = 1000;
   listLobsWithLimitAndCheckResult( cl, filePath, limitNum );

   //test c :listLobs with sort(), asce Order by Size
   var sortCond = { "Size": 1, "CreateTime": 1 };
   var sortKey = "Size";
   var sortOrder = 1;
   listLobsWithSortAndCheckResult( cl, filePath, sortCond, sortKey, sortOrder );

   //test c :listLobs with sort(), desc Order by Size; 
   var sortCond = { "Size": -1, "CreateTime": -1 };
   var sortKey = "Size";
   var sortOrder = -1;
   listLobsWithSortAndCheckResult( cl, filePath, sortCond, sortKey, sortOrder );

   //test d :listLobs with skip()
   var skipNum = 0;
   listLobsWithSkipAndCheckResult( cl, filePath, skipNum );

   //test d :listLobs with skip()
   var skipNum = 1000;
   listLobsWithSkipAndCheckResult( cl, filePath, skipNum );

   //test d :listLobs with skip()
   var skipNum = 1200;
   listLobsWithSkipAndCheckResult( cl, filePath, skipNum );

   //test c: listLobs with skip/limit/sort
   var skipNum = 300;
   var limitNum = 700;
   listLobsWithQueryOptionAndCheckResult( cl, filePath, skipNum, limitNum );

   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the ending" );
   deleteTmpFile( filePath );
}

function putLobs ( cl, filePath )
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
         var lobOid = cl.createLobID( beginDate );
         cl.putLob( filePath + fileName, lobOid );
         //truncateLob to construct a different size lob. taks less time than generating files of different sizes.
         cl.truncateLob( lobOid, lobSize );
      }
   }
}
