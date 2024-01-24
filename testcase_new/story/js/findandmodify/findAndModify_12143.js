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
   var clName = COMMCLNAME + "_12143";
   commDropCL( db, COMMCSNAME, clName );
   var maincl = commCreateCL( db, COMMCSNAME, clName, { IsMainCL: true, ShardingType: 'range', ShardingKey: { 'date': 1 } } );
   test_subclnonsplit( db, maincl, clName );
   test_subclsplit( db, maincl, clName );
   test_subclonsamegroup( db, maincl, clName );
   commDropCL( db, COMMCSNAME, clName );
}
/*******************************************************************************
*@Description：测试op为remove时, 主子表情况下结合skip, 结果不落在单张子表上
*@Input：find( {date:{$gte:20150101}} ).skip( 2 ).remove()
*@Expectation：报-289错误
********************************************************************************/

function test_UsedSkipOfFailed ( cl )
{
   try
   {
      loadMultipleDoc( cl, 5 * 4 );
      var arrdoc = cl.find( { date: { $gte: 20150101 } } ).skip( 2 ).remove().toArray();

      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( SDB_RTN_QUERYMODIFY_MULTI_NODES != e.message )
      {
         throw e;
      }
   }
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 主子表情况下结合limit，结果不落在单张子表上
*@Input：find( {date:{$gte:20150101}} ).limit( 2 ).remove()
*@Expectation：报-289错误
********************************************************************************/
function test_UsedLimitOfFailed ( cl )
{
   try
   {
      loadMultipleDoc( cl, 5 * 4 );
      var arrdoc = cl.find( { date: { $gte: 20150101 } } ).limit( 2 ).remove().toArray();

      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( SDB_RTN_QUERYMODIFY_MULTI_NODES != e.message )
      {
         throw e;
      }
   }
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为remove时, 主子表情况下结合skip + limit，结果不落在单张子表上
*@Input：find( {date:{$gte:20150101}} ).skip( 2 ).limit( 5 ).remove()
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipAndLimitOfFailed ( cl )
{
   try
   {
      loadMultipleDoc( cl, 5 * 4 );
      var arrdoc = cl.find( { date: { $gte: 20150101 } } ).skip( 2 ).limit( 5 ).remove().toArray();

      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( SDB_RTN_QUERYMODIFY_MULTI_NODES != e.message )
      {
         throw e;
      }
   }
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为remove时, 主子表情况下结合skip, 结果不落在单张子表上
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).remove()
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipOfFailedSplit ( cl )
{
   try
   {
      var groupNum = commGetGroupsNum( db );
      if( groupNum == 1 ) return;
      loadMultipleDoc( cl, 5 * 4 * groupNum );
      var arrdoc = cl.find( { $and: [{ date: { $gte: 20150101 } }, { date: { $lte: 20150201 } }] } ).skip( 2 ).remove().toArray();

      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( SDB_RTN_QUERYMODIFY_MULTI_NODES != e.message )
      {
         throw e;
      }
   }
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为remove时, 主子表情况下结合limit，结果不落在单个分区表上
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).limit( 2 ).remove()
*@Expectation：报-289错误
********************************************************************************/
function test_UsedLimitOfFailedSplit ( cl )
{
   try
   {
      var groupNum = commGetGroupsNum( db );
      if( groupNum == 1 ) return;
      loadMultipleDoc( cl, 5 * 4 * groupNum );
      var arrdoc = cl.find( { $and: [{ date: { $gte: 20150101 } }, { date: { $lt: 20150201 } }] } ).limit( 2 ).remove().toArray();

      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( SDB_RTN_QUERYMODIFY_MULTI_NODES != e.message )
      {
         throw e;
      }
   }
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为remove时, 主子表情况下结合skip + limit，结果不落在单分区上
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).limit( 5 ).remove()
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipAndLimitOfFailedSplit ( cl )
{
   try
   {
      var groupNum = commGetGroupsNum( db );
      if( groupNum == 1 ) return;
      loadMultipleDoc( cl, 5 * 4 * groupNum );
      var arrdoc = cl.find( { $and: [{ date: { $gte: 20150101 } }, { date: { $lte: 20150201 } }] } ).skip( 2 ).limit( 5 ).remove().toArray();

      throw new Error( "need throw error" );
   }
   catch( e )
   {
      if( SDB_RTN_QUERYMODIFY_MULTI_NODES != e.message )
      {
         throw e;
      }
   }
   cl.truncate();
}

function checkResult ( cl, totalnum, subnum )
{
   var totalcount = cl.find().count();
   var subcl0count = cl.find( { $and: [{ date: { $gte: 20150101 } }, { date: { $lt: 20150201 } }] } ).count();
   if( totalnum != parseInt( totalcount ) && subnum != parseInt( subcl0count ) )
   {
      throw new Error( "totalnum: " + totalnum + "\ntotalcount: " + totalcount + "\nsubnum: " + subnum + "\nsubcl0count: " + subcl0count );
   }
}

/*******************************************************************************
*@Description：测试op为remove时, 主子表情况下，子表不切分的情况下使用skip( 结果落在单张子表上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 5 ).remove()
*@Expectation：返回的文档已经被删除
********************************************************************************/
function test_UsedSkipOfSuccessNonSplit ( cl )
{
   loadMultipleDoc( cl, 5 * 4 );
   var arrdoc = cl.find( { $and: [{ date: { $gte: 20150101 } }, { date: { $lt: 20150201 } }] } ).skip( 1 ).remove().toArray();

   if( !checkRemoveResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   checkResult( cl, 16, 1 );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为remove时, 主子表情况下，子表不切分的情况下使用limit( 结果落在单张子表上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).limit( 5 ).remove()
*@Expectation：返回的文档已经被删除
********************************************************************************/
function test_UsedLimitOfSuccessNonSplit ( cl )
{
   loadMultipleDoc( cl, 5 * 4 );
   var arrdoc = cl.find( { $and: [{ date: { $gte: 20150101 } }, { date: { $lt: 20150201 } }] } ).limit( 5 ).remove().toArray();

   if( !checkRemoveResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }

   checkResult( cl, 15, 0 );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为remove时, 主子表情况下，子表不切分的情况下使用limit + skip( 结果落在单张子表上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, {date:{$lt:20150201}}]} ).skip( 2 ).limit( 2 ).remove()
*@Expectation：返回的文档已经被删除
********************************************************************************/
function test_UsedSkipAndLimitOfSuccessNonSplit ( cl )
{
   loadMultipleDoc( cl, 5 * 4 );
   var arrdoc = cl.find( { $and: [{ date: { $gte: 20150101 } }, { date: { $lt: 20150201 } }] } ).skip( 2 ).limit( 2 ).remove().toArray();

   if( !checkRemoveResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   checkResult( cl, 18, 3 );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为remove时, 主子表情况下，子表切分的情况下使用skip( 结果落在单分区上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, 
{date:{$lt:20150201}}, {_id:{$gte:0}}, {_id:{$lt:5}}]} ).
skip( 1 ).remove()
*@Expectation：返回的文档已经被删除
********************************************************************************/
function test_UsedSkipOfSuccessSplit ( cl )
{
   var totalnum = 5 * 4 * commGetGroupsNum( db );
   loadMultipleDoc( cl, totalnum );
   var arrdoc = cl.find( {
      $and: [{ date: { $gte: 20150101 } },
      { date: { $lt: 20150201 } }, { _id: { $gte: 0 } }, { _id: { $lt: 5 } }]
   } ).
      skip( 1 ).remove().toArray();

   if( !checkRemoveResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   checkResult( cl, totalnum - 4, 1 );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为remove时, 主子表情况下，子表切分的情况下使用limit( 结果落在分区上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, 
{date:{$lt:20150201}}, {_id:{$gte:0}}, {_id:{$lt:5}}]} ).
limit( 5 ).remove()
*@Expectation：返回的文档已经被删除
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
      limit( 5 ).remove().toArray();

   if( !checkRemoveResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   checkResult( cl, totalnum - 5, 0 );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为remove时, 主子表情况下，子表切分的情况下使用limit + skip( 结果落在单分区上 )
*@Input：find( {$and:[{date:{$gte:20150101}}, 
{date:{$lt:20150201}}, {_id:{$gte:0}}, {_id:{$lt:5}}]} ).
skip( 2 ).limit( 2 ).remove()
*@Expectation：返回的文档已经被删除
********************************************************************************/
function test_UsedSkipAndLimitOfSuccessSplit ( cl )
{
   var totalnum = 5 * 4 * commGetGroupsNum( db );
   loadMultipleDoc( cl, totalnum );
   var arrdoc = cl.find( {
      $and: [{ date: { $gte: 20150101 } },
      { date: { $lt: 20150201 } }, { _id: { $gte: 0 } }, { _id: { $lt: 5 } }]
   } ).
      skip( 2 ).limit( 2 ).remove().toArray();

   if( !checkRemoveResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   checkResult( cl, totalnum - 2, 3 );
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
   test_UsedSkipOfFailed( maincl );
   test_UsedLimitOfFailed( maincl );
   test_UsedSkipAndLimitOfFailed( maincl );
   test_UsedSkipOfFailedSplit( maincl );
   test_UsedLimitOfFailedSplit( maincl );
   test_UsedSkipAndLimitOfFailedSplit( maincl );
   test_UsedSkipOfSuccessSplit( maincl );
   test_UsedLimitOfSuccessSplit( maincl );
   test_UsedSkipAndLimitOfSuccessSplit( maincl );
   fini( db, clName );
}

function test_subclonsamegroup ( db, maincl, clName )
{
   var datagroups = commGetGroups( db, true );
   if( datagroups.length > 1 )
   {
      init( db, maincl, clName, false, datagroups[0][0].GroupName );
      test_UsedSkipOfFailed( maincl );
      test_UsedLimitOfFailed( maincl );
      test_UsedSkipAndLimitOfFailed( maincl );
      test_UsedSkipOfSuccessNonSplit( maincl );
      test_UsedLimitOfSuccessNonSplit( maincl );
      test_UsedSkipAndLimitOfSuccessNonSplit( maincl );
   }
   fini( db, clName );
}
