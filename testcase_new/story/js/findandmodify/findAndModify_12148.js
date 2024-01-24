/*******************************************************************************
*@Description: findandupdate basic testcases
*@Modify list:
*   2014-4-6 wenjing wang  Init
*******************************************************************************/

main( test );

function test ()
{
   if( true != commIsStandalone( db ) && commGetGroupsNum( db ) > 1 )
   {
      var clName = COMMCLNAME + "_12148";
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
*@Description：测试op为update时, 结合使用skip，结果不落在单个分区上
*@Input：find().skip( 5 ).update( {$set:{b:1}} )
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipOfFailed ( cl )
{
   assert.tryThrow( SDB_RTN_QUERYMODIFY_MULTI_NODES, function()
   {
      var loadnumber = 5 * commGetGroupsNum( db );
      loadMultipleDoc( cl, loadnumber );
      var arrdoc = cl.find().skip( 5 ).update( { $set: { b: 1 } } ).toArray();
   } );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 结合使用limit，结果不落在单个分区上
*@Input：find().limit( 5 ).update( {$set:{b:1}} )
*@Expectation：报-289错误
********************************************************************************/
function test_UsedLimitOfFailed ( cl )
{
   assert.tryThrow( SDB_RTN_QUERYMODIFY_MULTI_NODES, function()
   {
      var loadnumber = 5 * commGetGroupsNum( db );
      loadMultipleDoc( cl, loadnumber );
      var arrdoc = cl.find().limit( 5 ).update( { $set: { b: 1 } } ).toArray();
   } );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 结合使用skip + limit, 结果不落在单个分区上
*@Input：find().skip( 2 ).limit( 5 ).update( {$set:{b:1}} )
*@Expectation：报-289错误
********************************************************************************/
function test_UsedSkipAndLimitOfFailed ( cl )
{
   assert.tryThrow( SDB_RTN_QUERYMODIFY_MULTI_NODES, function()
   {
      var loadnumber = 5 * commGetGroupsNum( db );
      loadMultipleDoc( cl, loadnumber );
      var arrdoc = cl.find().skip( 2 ).limit( 5 ).update( { $set: { b: 1 } } ).toArray();
   } );
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 结合使用skip, 结果落在单个分区上
*@Input：find( {$and:[{_id:{$lt:5}}, {_id:{$gte:0}}]} ).limit( 2 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function test_UsedLimitOfSuccess ( cl )
{
   var loadnumber = 5 * commGetGroupsNum( db );
   loadMultipleDoc( cl, loadnumber );
   var arrdoc = cl.find( { $and: [{ _id: { $lt: 5 } }, { _id: { $gte: 0 } }] } ).skip( 2 ).update( { $set: { b: 1 } } ).toArray();

   if( !checkUpdateResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 结合使用limit, 结果落在单个分区上
*@Input：find( {$and:[{_id:{$lt:5}}, {_id:{$gte:0}}]} ).skip( 2 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function test_UsedSkipOfSuccess ( cl )
{
   var loadnumber = 5 * commGetGroupsNum( db );
   loadMultipleDoc( cl, loadnumber );
   var arrdoc = cl.find( { $and: [{ _id: { $lt: 5 } }, { _id: { $gte: 0 } }] } ).skip( 2 ).update( { $set: { b: 1 } } ).toArray();

   if( !checkUpdateResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   cl.truncate();
}

/*******************************************************************************
*@Description：测试op为update时, 结合使用skip + limit, 结果落在单个分区上
*@Input：find( {$and:[{_id:{$lt:5}}, {_id:{$gte:0}}]} ).skip( 2 ).limit( 2 ).update( {$set:{b:1}} )
*@Expectation：返回的文档能够查询出字段b的值为1
********************************************************************************/
function test_UsedSkipAndLimitOfSuccess ( cl )
{
   var loadnumber = 5 * commGetGroupsNum( db );
   loadMultipleDoc( cl, loadnumber );
   var arrdoc = cl.find( { $and: [{ _id: { $lt: 5 } }, { _id: { $gte: 0 } }] } ).skip( 2 ).limit( 2 ).update( { $set: { b: 1 } } ).toArray();

   if( !checkUpdateResult( cl, arrdoc ) )
   {
      throw new Error( "arrdoc: " + arrdoc );
   }
   cl.truncate();
}
