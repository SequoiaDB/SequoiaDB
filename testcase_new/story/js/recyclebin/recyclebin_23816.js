/******************************************************************************
 * @Description   : seqDB-23816:truncate后执行split，恢复/强制恢复truncate
 * @Author        : liuli
 * @CreateTime    : 2021.04.09
 * @LastEditTime  : 2022.06.28
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipGroupLessThanThree = true;
main( test );
function test ()
{
   var csName = "cs_23816";
   var clName1 = "cl_23816_1";
   var clName2 = "cl_23816_2";
   var clName3 = "cl_23816_3";
   var groupNames = commGetDataGroupNames( db );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );

   // 1、CL在group1，split部分数据到group2
   var dbcl = dbcs.createCL( clName1, { ShardingKey: { a: 1 }, Group: groupNames[0] } );
   var docs = insertBulkData( dbcl, 1000 );
   var expectResult = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + clName1 } ).current().toObj().CataInfo;

   // truncate后split
   dbcl.truncate();
   dbcl.split( groupNames[0], groupNames[1], 50 );

   // 恢复truncate项目
   var recycleName = getOneRecycleName( db, csName + "." + clName1, "Truncate" );
   assert.tryThrow( [SDB_RECYCLE_CONFLICT], function() 
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 强制恢复truncate项目
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );

   // 直连data校验数据
   var data = db.getRG( groupNames[0] ).getMaster().connect();
   var cursor = data.getCS( csName ).getCL( clName1 ).find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   // 校验恢复后cl所属的group
   var actualResult = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + clName1 } ).current().toObj().CataInfo;
   assert.equal( expectResult, actualResult, "Collection metadata information is incorrect ,expect : " + JSON.stringify( expectResult, 0, 2 ) + " actual : " + JSON.stringify( actualResult, 0, 2 ) );
   data.close();

   // 2、CL在group1，split全部数据到group2
   var dbcl = dbcs.createCL( clName2, { ShardingKey: { a: 1 }, Group: groupNames[0] } );
   var docs = insertBulkData( dbcl, 1000 );
   var expectResult = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + clName2 } ).current().toObj().CataInfo;

   // truncate后split
   dbcl.truncate();
   dbcl.split( groupNames[0], groupNames[1], 100 );

   // 恢复truncate项目
   var recycleName = getOneRecycleName( db, csName + "." + clName2, "Truncate" );
   assert.tryThrow( [SDB_RECYCLE_CONFLICT], function() 
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 强制恢复truncate项目
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );

   // 直连data校验数据
   var data = db.getRG( groupNames[0] ).getMaster().connect();
   var cursor = data.getCS( csName ).getCL( clName2 ).find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   // 校验恢复后cl所属的group
   var actualResult = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + clName2 } ).current().toObj().CataInfo;
   assert.equal( expectResult, actualResult, "Collection metadata information is incorrect ,expect : " + JSON.stringify( expectResult, 0, 2 ) + " actual : " + JSON.stringify( actualResult, 0, 2 ) );
   data.close();

   // 3、CL在group1和group2，group1 split全部数据到group2
   var dbcl = dbcs.createCL( clName3, { ShardingKey: { a: 1 }, Group: groupNames[0] } );
   dbcl.split( groupNames[0], groupNames[1], 50 );
   var docs = insertBulkData( dbcl, 1000 );
   var expectResult = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + clName3 } ).current().toObj().CataInfo;

   // truncate后split
   dbcl.truncate();
   dbcl.split( groupNames[1], groupNames[0], 100 );

   // 恢复truncate项目
   var recycleName = getOneRecycleName( db, csName + "." + clName3, "Truncate" );
   assert.tryThrow( [SDB_RECYCLE_CONFLICT], function() 
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 强制恢复truncate项目
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );

   // 校验数据
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   // 校验恢复后cl所属的group
   var actualResult = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + clName3 } ).current().toObj().CataInfo;
   assert.equal( expectResult, actualResult, "Collection metadata information is incorrect ,expect : " + JSON.stringify( expectResult, 0, 2 ) + " actual : " + JSON.stringify( actualResult, 0, 2 ) );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}