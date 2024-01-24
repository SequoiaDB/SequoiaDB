/******************************************************************************
 * @Description   : seqDB-24288:一个子表使用数据源，删除主表后数据源端被映射的表丢失恢复主表
 * @Author        : liuli
 * @CreateTime    : 2021.07.12
 * @LastEditTime  : 2022.08.15
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc24288";
   var csName = "cs_24288";
   var clName = "cl_24288";
   var srcCSName = "datasrcCS_24288";
   var mainCLName = "mainCL_24288";
   var subCLName = "subCL_24288";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   cleanRecycleBin( db, csName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl );
   var dsVersion = getDSVersion( dataSrcName );

   if( dsVersion[0] >= 3 && dsVersion[1] >= 6 )
   {
      cleanRecycleBin( datasrcDB, srcCSName );
   }

   var dbcs = db.createCS( csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( csName + "." + subCLName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );
   var docs = [];
   for( var i = 0; i < 1500; i++ )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs.push( { a: i, b: bValue, c: cValue } );
   }
   mainCL.insert( docs );

   // 删除主表后删除数据源端映射的表，恢复主表
   dbcs.dropCL( mainCLName );
   datasrcDB.getCS( srcCSName ).dropCL( clName );
   var originName = csName + "." + mainCLName;
   var recycleName = getOneRecycleName( db, originName );
   db.getRecycleBin().returnItem( recycleName );

   // 校验恢复后主子表对应关系及分区范围
   var subCLNames = [csName + "." + subCLName, csName + "." + clName];
   var shardRanges = [0, 1000, 2000];
   checkSubCL( csName, mainCLName, subCLNames, shardRanges );
   checkRecycleItem( recycleName );

   // 在主表执行数据操作
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      mainCL.insert( [{ a: 1, b: 1 }, { a: 2, b: "tests2", c: "test" }] );
   } );

   // 在使用数据源的子表执行数据操作
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.find().toArray();
   } );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   cleanRecycleBin( db, csName );
   if( dsVersion[0] >= 3 && dsVersion[1] >= 6 )
   {
      cleanRecycleBin( datasrcDB, srcCSName );
   }
   datasrcDB.close();
}