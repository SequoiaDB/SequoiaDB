/******************************************************************************
 * @Description   : seqDB-24094:分区表，删除分区表，重建分区表后回收分区表
 * @Author        : liuli
 * @CreateTime    : 2021.04.15
 * @LastEditTime  : 2022.06.28
 * @LastEditors   : liuli
 ******************************************************************************/
// 分区范围与原表分区范围相同
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_24094a";
   var clName = "cl_24094a";
   var originName = csName + "." + clName;
   var groupNames = commGetDataGroupNames( db );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 }, ShardingType: "range", Group: groupNames[0] } )
   dbcl.split( groupNames[0], groupNames[1], { "a": 100 }, { "a": 200 } );

   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   dbcl.insert( docs );
   var expectResult = db.snapshot( SDB_SNAP_CATALOG, { Name: originName } );

   // 删除CL后创建同名CL分区范围相同
   dbcs.dropCL( clName );
   var dbcl = dbcs.createCL( clName, { ShardingKey: { a: 1 }, ShardingType: "range", Group: groupNames[0] } )
   dbcl.split( groupNames[0], groupNames[1], { "a": 100 }, { "a": 200 } );

   // 恢复CL
   var recycleName = getOneRecycleName( db, originName, "Drop" );
   assert.tryThrow( [SDB_RECYCLE_CONFLICT], function() 
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 强制恢复CL
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   // 校验分区范围
   var actualResult = db.snapshot( SDB_SNAP_CATALOG, { Name: originName } );
   checkSplitRange( expectResult, actualResult );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}

function checkSplitRange ( expectResult, actualResult )
{
   var message = "The split range is inconsistent before and after return !"
   while( expectResult.next() )
   {
      actualResult.next();
      var actCataInfo = actualResult.current().toObj().CataInfo;
      var expCataInfo = expectResult.current().toObj().CataInfo;
      if( actCataInfo.length != expCataInfo.length )
      {
         throw new Error( message + " expect : " + JSON.stringify( expCataInfo, 0, 2 ) + " actual : " + JSON.stringify( actCataInfo, 0, 2 ) );
      }
      for( var i = 0; i < actCataInfo.length; i++ )
      {
         if( !commCompareObject( expCataInfo[i], actCataInfo[i] ) )
         {
            throw new Error( message + " expect : " + JSON.stringify( expCataInfo, 0, 2 ) + " actual : " + JSON.stringify( actCataInfo, 0, 2 ) );
         }
      }
   }
   expectResult.close();
}