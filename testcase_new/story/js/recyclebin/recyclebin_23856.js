/******************************************************************************
 * @Description   : seqDB-23856:有dropCL回收项目，创建同名CL
 * @Author        : liuli
 * @CreateTime    : 2021.04.19
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_23856";
   var clName = "cl_23856";
   var originName = csName + "." + clName;

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 }, AutoSplit: true } );

   var docs1 = [];
   for( var i = 0; i < 1000; i++ )
   {
      var bValue = parseInt( Math.random() * 5000 );
      docs1.push( { a: i, b: bValue } );
   }
   dbcl.insert( docs1 );

   // 删除CL后创建同名CL，并插入数据
   dbcs.dropCL( clName );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { b: 1 }, AutoSplit: true } );
   var docs2 = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs2.push( { b: i, c: i } );
   }
   dbcl.insert( docs2 );

   var recycleName = getOneRecycleName( db, originName, "Drop" );
   // 恢复dropCL项目
   assert.tryThrow( [SDB_RECYCLE_CONFLICT], function() 
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 强制恢复dropCL项目
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );

   // 校验CL数据及元数据正确性
   var cursor = dbcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs1 );
   checkRecycleItem( recycleName );

   var expShardingKey = { a: 1 };
   var actShardingKey = db.snapshot( SDB_SNAP_CATALOG, { Name: originName } ).current().toObj()["ShardingKey"];
   if( !commCompareObject( expShardingKey, actShardingKey ) )
   {
      throw new Error( "Sharding Key Error ,expect : " + JSON.stringify( expShardingKey ) + " actual : " + JSON.stringify( actShardingKey ) );
   }

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}