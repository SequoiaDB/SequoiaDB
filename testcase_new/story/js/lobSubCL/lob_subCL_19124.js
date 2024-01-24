/************************************
*@Description:  seqDB-19124 执行listLobs查询匹配不到记录
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
   var mainCLName = "mainCL19124";
   var subCLName = "subcl19124";
   var subCLNum = 2;
   var filePath = WORKDIR + "/subCLLob19124/";
   deleteTmpFile( filePath );
   var scope = 5;
   var beginBound = 20190101;
   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the beginning" );
   var mainCL = createMainCLAndAttachCL( COMMCSNAME, mainCLName, subCLName, subCLNum, beginBound, scope );
   splitSubCL( COMMCSNAME, subCLName + "_1" );

   //put lob
   var lobSizes = [1024, 10, 36, 1024 * 10, 1024 * 95, 1024 * 20, 3, 1, 10, 0];
   for( var i = 0; i < lobSizes.length; ++i )
   {
      var fileName = "lob_" + lobSizes[i];
      makeTmpFile( filePath, fileName, lobSizes[i] );
      var beginDate = beginBound + i;
      insertLob( mainCL, filePath + fileName, "YYYYMMDD", scope, 1, 1, beginDate );
   }

   var condition = { "Size": 2 };
   //test a: 主表上执行listLobs查询
   listLobs( COMMCSNAME, mainCLName, condition );

   //test b: 普通表subcl1上执行listLobs查询
   listLobs( COMMCSNAME, subCLName + "_0", condition );

   //test c: 切分表subcl2上执行listLobs查询
   listLobs( COMMCSNAME, subCLName + "_1", condition );

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
      if( i == 0 )
      {
         commCreateCL( db, csName, subCLName );
      }
      else
      {
         commCreateCL( db, csName, subCLName, options );
      }
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

function listLobs ( csName, clName, condition )
{
   var dbcl = db.getCS( csName ).getCL( clName );
   var listResult = dbcl.listLobs( SdbQueryOption().cond( condition ) );
   var actResult = [];
   while( listResult.next() )
   {
      var listObj = listResult.current().toObj();
      actRecs.push( listObj );
   }
   if( actResult.length !== 0 )
   {
      throw new Error( "checkRec()", "\nactual value= " + JSON.stringify( actRecs ) + "\ncondition= " + JSON.stringify( condition ) );
   }
}
