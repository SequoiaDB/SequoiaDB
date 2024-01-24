/*******************************************************************************
*@Description: findandupdate basic testcases
*@Modify list:
*   2014-4-8 wenjing wang  Init
*******************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   var clName = COMMCLNAME + "_12147";
   commDropCL( db, COMMCSNAME, clName );
   var maincl = commCreateCL( db, COMMCSNAME, clName, { IsMainCL: true, ShardingType: 'range', ShardingKey: { 'date': 1 } } );
   test_subclnonsplit( db, maincl, clName );
   test_subclsplit( db, maincl, clName );
   test_subclonsamegroup( db, maincl, clName );
   commDropCL( db, COMMCSNAME, clName );
}
/*******************************************************************************
*@Description：测试op为update时, 主子表情况下结合skip, 结果不落在单张子表上
*@Input：find( {date:{$gte:20150101}} ).skip( 5 ).update( {$set:{b:1}} )
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipOfFailed ( cl )
{
   assert.tryThrow( SDB_RTN_QUERYMODIFY_MULTI_NODES, function()
   {
      loadMultipleDoc( cl, 5 * 4 );
      var arrdoc = cl.find( { date: { $gte: 20150101 } } ).skip( 2 ).update( { $set: { b: 1 } } ).toArray();
   } );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下结合limit，结果不落在单张子表上
*@Input：find( {date:{$gte:20150101}} ).limit( 5 ).update( {$set:{b:1}} )
*@Expectation：报-289错误
********************************************************************************/
function test_UsedLimitOfFailed ( cl )
{
   assert.tryThrow( SDB_RTN_QUERYMODIFY_MULTI_NODES, function()
   {
      loadMultipleDoc( cl, 5 * 4 );
      var arrdoc = cl.find( { date: { $gte: 20150101 } } ).limit( 2 ).update( { $set: { b: 1 } } ).toArray();
   } );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下结合skip + limit，结果不落在单张子表上
*@Input：find( {date:{$gte:20150101}} ).skip( 2 ).limit( 5 ).update( {$set:{b:1}} )
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipAndLimitOfFailed ( cl )
{
   assert.tryThrow( SDB_RTN_QUERYMODIFY_MULTI_NODES, function()
   {
      loadMultipleDoc( cl, 5 * 4 );
      var arrdoc = cl.find( { date: { $gte: 20150101 } } ).skip( 2 ).limit( 5 ).update( { $set: { b: 1 } } ).toArray();
   } );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下结合skip, 子表切分，结果不落在单分区上
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).update( {$set:{b:1}} )
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipOfFailedSplit ( cl )
{
   assert.tryThrow( SDB_RTN_QUERYMODIFY_MULTI_NODES, function()
   {
      var groupNum = commGetGroupsNum( db );
      if( groupNum == 1 ) return;
      loadMultipleDoc( cl, 5 * 4 * groupNum );
      var arrdoc = cl.find( { $and: [{ date: { $gte: 20150101 } }, { date: { $lt: 20150201 } }] } ).skip( 2 ).update( { $set: { b: 1 } } ).toArray();
   } );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下结合limit，子表切分，结果不落在单分区上
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).limit( 2 ).update( {$set:{b:1}} )
*@Expectation：报-289错误
********************************************************************************/
function test_UsedLimitOfFailedSplit ( cl )
{
   assert.tryThrow( SDB_RTN_QUERYMODIFY_MULTI_NODES, function()
   {
      var groupNum = commGetGroupsNum( db );
      if( groupNum == 1 ) return;
      loadMultipleDoc( cl, 5 * 4 * groupNum );
      var arrdoc = cl.find( { $and: [{ date: { $gte: 20150101 } }, { date: { $lt: 20150201 } }] } ).limit( 2 ).update( { $set: { b: 1 } } ).toArray();
   } );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下结合skip + limit，子表切分，结果不落在单分区上
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).limit( 5 ).update( {$set:{b:1}} )
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipAndLimitOfFailedSplit ( cl )
{
   assert.tryThrow( SDB_RTN_QUERYMODIFY_MULTI_NODES, function()
   {
      var groupNum = commGetGroupsNum( db );
      if( groupNum == 1 ) return;
      loadMultipleDoc( cl, 5 * 4 * commGetGroupsNum( db ) );
      var arrdoc = cl.find( { $and: [{ date: { $gte: 20150101 } }, { date: { $lt: 20150201 } }] } ).skip( 2 ).limit( 5 ).update( { $set: { b: 1 } } ).toArray();
   } );
   cl.truncate();
}

function checkResult ( cl, updatenum )
{
   var updatecount = cl.find( { b: 1 } ).count();
   if( updatenum != parseInt( updatecount ) )
   {
      throw new Error( "updatenum: " + updatenum + "\nupdatecount: " + updatecount );
   }
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下，子表不切分的情况下使用skip( 结果落在单张子表上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 1 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function test_UsedSkipOfSuccessNonSplit ( cl )
{
   loadMultipleDoc( cl, 4 * 5 );
   var arrdoc = cl.find( { $and: [{ date: { $gte: 20150101 } }, { date: { $lt: 20150201 } }] } ).skip( 1 ).update( { $set: { b: 1 } } ).toArray();

   if( !checkUpdateResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   checkResult( cl, 4 );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下，子表不切分的情况下使用limit( 结果落在单张子表上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).limit( 5 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function test_UsedLimitOfSuccessNonSplit ( cl )
{
   loadMultipleDoc( cl, 5 * 4 );
   var arrdoc = cl.find( { $and: [{ date: { $gte: 20150101 } }, { date: { $lt: 20150201 } }] } ).limit( 5 ).update( { $set: { b: 1 } } ).toArray();

   if( !checkUpdateResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   checkResult( cl, 5 );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下，子表不切分的情况下使用limit + skip( 结果落在单张子表上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).limit( 2 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function test_UsedSkipAndLimitOfSuccessNonSplit ( cl )
{
   loadMultipleDoc( cl, 5 * 4 );
   var arrdoc = cl.find( { $and: [{ date: { $gte: 20150101 } }, { date: { $lt: 20150201 } }] } ).skip( 2 ).limit( 2 ).update( { $set: { b: 1 } } ).toArray();

   if( !checkUpdateResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   checkResult( cl, 2 );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下，子表切分的情况下使用skip( 结果落在单分区上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, 
{date:{$lt:20150201}}, {_id:{$gte:0}}, {_id:{$lt:5}}]} ).
skip( 1 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function test_UsedSkipOfSuccessSplit ( cl )
{
   var totalnum = 5 * 4 * commGetGroupsNum( db );
   loadMultipleDoc( cl, totalnum );
   var arrdoc = cl.find( {
      $and: [{ date: { $gte: 20150101 } },
      { date: { $lt: 20150201 } }, { _id: { $gte: 0 } }, { _id: { $lt: 5 } }]
   } ).
      skip( 1 ).update( { $set: { b: 1 } } ).toArray();

   if( !checkUpdateResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   checkResult( cl, 4 );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下，子表切分的情况下使用limit( 结果落在单分区上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, 
{date:{$lt:20150201}}, {_id:{$gte:0}}, {_id:{$lt:5}}]} ).
limit( 5 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function test_UsedLimitOfSuccessSplit ( cl )
{
   var totalnum = 5 * 4 * commGetGroupsNum( db );
   loadMultipleDoc( cl, totalnum );
   var arrdoc = cl.find( {
      $and: [{ date: { $gte: 20150101 } },
      { date: { $lt: 20150201 } },
      { _id: { $gte: 0 } }, { _id: { $lt: 5 } }]
   } ).
      limit( 5 ).update( { $set: { b: 1 } } ).toArray();

   if( !checkUpdateResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   checkResult( cl, 5 );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下，子表切分的情况下使用limit + skip( 结果落在单分区上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, 
{date:{$lt:20150201}}, {_id:{$gte:0}}, {_id:{$lt:5}}]} ).
skip( 2 ).limit( 2 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function test_UsedSkipAndLimitOfSuccessSplit ( cl )
{
   var totalnum = 5 * 4 * commGetGroupsNum( db );
   loadMultipleDoc( cl, totalnum );
   var arrdoc = cl.find( {
      $and: [{ date: { $gte: 20150101 } },
      { date: { $lt: 20150201 } }, { _id: { $gte: 0 } }, { _id: { $lt: 5 } }]
   } ).
      skip( 2 ).limit( 2 ).update( { $set: { b: 1 } } ).toArray();

   if( !checkUpdateResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   checkResult( cl, 2 );
   cl.truncate();
}

function test_subclnonsplit ( db, maincl, clName )
{
   init( db, maincl, clName )
   test_UsedSkipOfFailed( maincl );
   test_UsedLimitOfFailed( maincl );
   test_UsedSkipAndLimitOfFailed( maincl );
   test_UsedSkipOfSuccessNonSplit( maincl );
   test_UsedLimitOfSuccessNonSplit( maincl );
   test_UsedSkipAndLimitOfSuccessNonSplit( maincl );
   fini( db, clName );
}

function test_subclsplit ( db, maincl, clName )
{
   init( db, maincl, clName, true );
   if( commGetGroupsNum( db ) > 1 )
   {
      test_UsedSkipOfFailed( maincl );
      test_UsedLimitOfFailed( maincl );
      test_UsedSkipAndLimitOfFailed( maincl );
      test_UsedSkipOfFailedSplit( maincl );
      test_UsedLimitOfFailedSplit( maincl );
      test_UsedSkipAndLimitOfFailedSplit( maincl );
      test_UsedSkipOfSuccessSplit( maincl );
      test_UsedLimitOfSuccessSplit( maincl );
      test_UsedSkipAndLimitOfSuccessSplit( maincl );
   }
   fini( db, clName );
}

function test_subclonsamegroup ( db, maincl, clName )
{
   var datagroups = commGetGroups( db, true );
   init( db, maincl, clName, false, datagroups[0][0].GroupName );
   test_UsedSkipOfFailed( maincl );
   test_UsedLimitOfFailed( maincl );
   test_UsedSkipAndLimitOfFailed( maincl );
   test_UsedSkipOfSuccessNonSplit( maincl );
   test_UsedLimitOfSuccessNonSplit( maincl );
   test_UsedSkipAndLimitOfSuccessNonSplit( maincl );
   fini( db, clName );
}
