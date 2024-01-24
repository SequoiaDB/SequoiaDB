/************************************
*@Description:  seqDB-19127 主表上执行query/limit/sort/skip/hint查询，查询数据跨多个子表多个组
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
   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }

   var mainCLName = "mainCL19127";
   var subCLName = "subcl19127";
   var subCLNum = 3;
   var filePath = WORKDIR + "/subCLLob19127/";
   deleteTmpFile( filePath );
   var scope = 10;
   var beginBound = 20190101;
   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the beginning" );
   var mainCL = createMainCLAndAttachCL( db, COMMCSNAME, mainCLName, subCLName, "YYYYMMDD", subCLNum, beginBound, scope );
   splitSubCL( COMMCSNAME, subCLName + "_2" );
   //put lob
   putLobs( mainCL, filePath );

   //listLobs with skip/limit/sort, query for all subcls
   var skipNum = 300;
   var limitNum = 1000;
   listLobsWithQueryOptionAndCheckResult( mainCL, filePath, skipNum, limitNum );

   //listLobs with skip/limit/sort, query for one subcl
   var skipNum = 1000;
   var limitNum = 20;
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
         //put lob in subcl1
         beginDate = "2019-01-11-00.00.00.000000";
      }
      else
      {
         //put lob in subcl2
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

function splitSubCL ( csName, subCLName )
{
   var clFullName = csName + "." + subCLName;
   var srcGroupName = commGetCLGroups( db, clFullName )[0];
   var targetGroupName = getTargetGroup( csName, subCLName, srcGroupName );
   var dbcl = db.getCS( csName ).getCL( subCLName );
   dbcl.split( srcGroupName, targetGroupName, 50 );
}
