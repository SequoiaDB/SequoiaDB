/******************************************************************************
 * @Description   : seqDB-23818:truncate后split所有数据到新组，恢复/强制恢复truncate
 * @Author        : liuli
 * @CreateTime    : 2021.04.09
 * @LastEditTime  : 2022.08.15
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipGroupLessThanThree = true;

main( test );
function test ()
{
   var csName = "cs_23818";
   var clName = "cl_23818";
   var originName = csName + "." + clName;
   var groupNames = commGetDataGroupNames( db );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 }, Group: groupNames[0] } )

   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   dbcl.insert( docs );
   dbcl.split( groupNames[0], groupNames[1], 50 );
   var expectResult = db.snapshot( SDB_SNAP_CATALOG, { Name: originName } ).current().toObj().CataInfo;

   // truncate后split部分数据到新组
   dbcl.truncate();
   dbcl.split( groupNames[0], groupNames[2], 100 );

   // 恢复truncate项目
   var recycleName = getOneRecycleName( db, originName, "Truncate" );
   assert.tryThrow( [SDB_RECYCLE_CONFLICT], function() 
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 强制恢复truncate项目
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   // 强制校验恢复后cl所属的group
   var actualResult = db.snapshot( SDB_SNAP_CATALOG, { Name: originName } ).current().toObj().CataInfo;
   if( !commCompareObject( expectResult, actualResult ) )
   {
      throw new Error( "Collection metadata information is incorrect ,expect : " + JSON.stringify( expectResult, 0, 2 ) + " actual : " + JSON.stringify( actualResult, 0, 2 ) );
   }

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}