/************************************
*@Description:  seqDB-19119 主表上listLobs使用include/add/subtract选择符查询，匹配数据跨子表
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

   if( commGetGroupsNum( db ) < 2 )
   {
      return;
   }
   var mainCLName = "mainCL19119";
   var subCLName = "subcl19119";
   var subCLNum = 2;
   var filePath = WORKDIR + "/subCLLob19119/";
   deleteTmpFile( filePath );
   var scope = 10;
   var beginBound = 20191001;
   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the beginning" );
   var mainCL = createMainCLAndAttachCL( COMMCSNAME, mainCLName, subCLName, subCLNum, beginBound, scope );
   splitSubCL( COMMCSNAME, subCLName + "_1" );

   //put lob
   var lobSizes = [1024, 10, 36, 1024 * 10, 1024 * 95, 1024 * 20, 3, 1, 10, 0, 1024 * 1024 * 8, 1024 * 2, 36, 1024 * 30, 1024 * 55, 1024 * 80, 1024 * 1024 * 3, 99, 80, 2];
   for( var i = 0; i < lobSizes.length; ++i )
   {
      var fileName = "lob_" + lobSizes[i];
      makeTmpFile( filePath, fileName, lobSizes[i] );
      var beginDate = beginBound + i;
      insertLob( mainCL, filePath + fileName, "YYYYMMDD", scope, 1, 1, beginDate );
   }

   //listLobs use $include
   var selSymbol = "$include";
   var condition = { "Size": { "$lte": 1024 * 1024 * 3 } };
   var selCondition = { "Size": { "$include": 1 } };
   listLobsWithSelCondAndCheckResult( mainCL, selSymbol, selCondition, condition );

   //listLobs use $add
   var modifyValue = 1024 * 10;
   var selSymbol = "$add";
   var condition = { "Size": { "$gte": 1024 * 1024 * 20 } };
   var selCondition = { "Size": { "$add": modifyValue } };
   listLobsWithSelCondAndCheckResult( mainCL, selSymbol, selCondition, condition, modifyValue );

   //listLobs use $subtract
   var modifyValue = 10;
   var selSymbol = "$subtract";
   var condition = { "Size": { "$gt": 36 } };
   var selCondition = { "Size": { "$subtract": modifyValue } };
   listLobsWithSelCondAndCheckResult( mainCL, selSymbol, selCondition, condition, modifyValue );

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the ending" );
   deleteTmpFile( filePath );
}

function createMainCLAndAttachCL ( csName, mainCLName, clName, subCLNum, beginBound, scope )
{
   var options = { "IsMainCL": true, "ShardingKey": { "date": 1 }, "LobShardingKeyFormat": "YYYYMMDD", "ShardingType": "range" };
   var mainCL = commCreateCL( db, csName, mainCLName, options, true, false, "create main cl" );

   var options = { ShardingKey: { _id: 1 }, ShardingType: "hash", Compressed: true };
   for( var i = 0; i < subCLNum; i++ )
   {
      var subCLName = clName + "_" + i;
      commCreateCL( db, csName, subCLName, options );
      var lowBound = { "date": ( parseInt( beginBound ) + i * scope ) + '' };
      var upBound = { "date": ( parseInt( beginBound ) + ( i + 1 ) * scope ) + '' };
      mainCL.attachCL( csName + "." + subCLName, { "LowBound": lowBound, "UpBound": upBound } );
   }
   return mainCL;
}

function splitSubCL ( csName, subCLName )
{
   var clFullName = csName + "." + subCLName;
   var srcGroupName = commGetCLGroups( db, clFullName )[0];
   var targetGroupName = getTargetGroup( csName, subCLName, srcGroupName );
   var dbcl = db.getCS( csName ).getCL( subCLName );
   dbcl.split( srcGroupName, targetGroupName, 50 );
}
