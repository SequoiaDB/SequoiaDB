/*******************************************************************************
*@Description: findandupdate basic testcases
*@Modify list:
*   2014-4-3 wenjing wang  Init
*******************************************************************************/

main( test );

function test ()
{
   if( true != commIsStandalone( db ) && commGetGroupsNum( db ) > 1 )
   {
      var clName = COMMCLNAME + "_12144";
      commDropCL( db, COMMCSNAME, clName );
      var cl = commCreateCL( db, COMMCSNAME, clName, { ShardingType: 'range', ShardingKey: { '_id': 1 } } );
      splittable( db, cl, clName );
      test_UsedSkipOfFailed( cl );
      test_UsedLimitOfFailed( cl );
      test_UsedSkipAndLimitOfFailed( cl );
      test_UsedSkipOfSuccess( cl );
      test_UsedLimitOfSuccess( cl );
      test_UsedSkipAndLimitOfSuccess( cl );
      commDropCL( db, COMMCSNAME, clName );
   }
}

/*******************************************************************************
*@Description：测试op为remove时, 使用skip，结果不落在单个分区上
*@Input：find().skip( 5 )，
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipOfFailed ( cl )
{
   try
   {
      var loadnumber = 5 * commGetGroupsNum( db );
      loadMultipleDoc( cl, loadnumber );
      var arrdoc = cl.find().skip( 5 ).remove().toArray();

      throw new error( "need throw error" );
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
*@Description：测试op为remove时, 结合limit，结果不落在单个分区上
*@Input: find().limit( 5 )
*@Expectation：报-289错误
********************************************************************************/
function test_UsedLimitOfFailed ( cl )
{
   try
   {
      var loadnumber = 5 * commGetGroupsNum( db );
      loadMultipleDoc( cl, loadnumber );
      var arrdoc = cl.find().limit( 5 ).remove().toArray();

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
*@Description：测试op为remove时, 给合使用skip + limit, 结果不落在单个分区上
*@Input：find().skip( 2 ).limit( 2 )
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipAndLimitOfFailed ( cl )
{
   try
   {
      var loadnumber = 5 * commGetGroupsNum( db );
      loadMultipleDoc( cl, loadnumber );
      var arrdoc = cl.find().skip( 2 ).limit( 2 ).remove().toArray();

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
*@Description：测试op为remove时, 结合使用limit，结果落在单个分区上
*@Input：find( {$and:[{_id:{$lt:5}}, {_id:{$gte:0}}]} ).limit( 2 )，
*@Expectation：返回的文档在主节点上查询不到
********************************************************************************/
function test_UsedLimitOfSuccess ( cl )
{
   var loadnumber = 5 * commGetGroupsNum( db );
   loadMultipleDoc( cl, loadnumber );
   var arrdoc = cl.find( { $and: [{ _id: { $lt: 5 } }, { _id: { $gte: 0 } }] } ).limit( 2 ).remove().toArray();

   if( !checkRemoveResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为remove时, 结合使用skip，结果落在单个分区上
*@Input：find( {$and:[{_id:{$lt:5}}, {_id:{$gte:0}}]} ).skip( 2 )
*@Expectation：返回的文档在主节点上查询不到
********************************************************************************/
function test_UsedSkipOfSuccess ( cl )
{
   var loadnumber = 5 * commGetGroupsNum( db );
   loadMultipleDoc( cl, loadnumber );
   var arrdoc = cl.find( { $and: [{ _id: { $lt: 5 } }, { _id: { $gte: 0 } }] } ).skip( 2 ).remove().toArray();

   if( !checkRemoveResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 结合skip + limit，结果落在单个分区上
*@Input：find( {$and:[{_id:{$lt:5}}, {_id:{$gte:0}}]} ).skip( 2 ).limit( 2 )
*@Expectation：返回的文档在主节点上查询不到
********************************************************************************/
function test_UsedSkipAndLimitOfSuccess ( cl )
{
   var loadnumber = 5 * commGetGroupsNum( db );
   loadMultipleDoc( cl, loadnumber );
   var arrdoc = cl.find( { $and: [{ _id: { $lt: 5 } }, { _id: { $gte: 0 } }] } ).skip( 2 ).limit( 2 ).remove().toArray();

   if( !checkRemoveResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   cl.truncate();
}
