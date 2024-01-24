/******************************************************************************
 * @Description   : seqDB-23857:有dropCS回收项目，创建同名CS
 * @Author        : liuli
 * @CreateTime    : 2021.04.19
 * @LastEditTime  : 2022.06.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_23857";
   var clName = "cl_23857";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var groupNames = commGetDataGroupNames( db );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 }, AutoSplit: true } );

   var docs1 = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs1.push( { a: i, b: i } );
   }
   dbcl.insert( docs1 );

   // 删除CS后创建同名CS，创建分区表并插入数据
   db.dropCS( csName );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { Group: groupNames[0], ShardingKey: { c: 1 } } );
   var docs2 = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs2.push( { c: i, d: i } );
   }
   dbcl.insert( docs2 );
   dbcl.split( groupNames[0], groupNames[1], 50 );

   var recycleName = getOneRecycleName( db, csName, "Drop" );
   // 恢复dropCS项目
   assert.tryThrow( [SDB_RECYCLE_CONFLICT], function() 
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 强制恢复dropCS项目
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );

   // 校验CL数据及元数据正确性
   var cursor = dbcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs1 );
   checkRecycleItem( recycleName );

   var expShardingKey = { a: 1 };
   var actShardingKey = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + clName } ).current().toObj()["ShardingKey"];
   if( !commCompareObject( expShardingKey, actShardingKey ) )
   {
      throw new Error( "Sharding Key Error ,expect : " + JSON.stringify( expShardingKey ) + " actual : " + JSON.stringify( actShardingKey ) );
   }

   var actCSGroup = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { Name: csName } ).current().toObj()["Group"];
   assert.equal( groupNames.sort(), actCSGroup.sort() );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}