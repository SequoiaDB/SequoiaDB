/******************************************************************************
 * @Description   : seqDB-24110:主子表一个子表使用数据源，删除主表
 * @Author        : liuli
 * @CreateTime    : 2021.04.21
 * @LastEditTime  : 2022.08.15
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc24110";
   var csName = "cs_24110";
   var clName = "cl_24110";
   var srcCSName = "datasrcCS_24110";
   var mainCLName = "mainCL_24110";
   var subCLName = "subCL_24110";

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
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( csName + "." + subCLName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   // 删除主表后删除数据源并恢复
   dbcs.dropCL( mainCLName );
   var originName = csName + "." + mainCLName;
   db.dropDataSource( dataSrcName );
   var recycleName = getOneRecycleName( db, originName );
   db.getRecycleBin().returnItem( recycleName );

   // 校验恢复后主子表对应关系及分区范围
   var subCLNames = [csName + "." + subCLName];
   var shardRanges = [1000, 2000];
   checkSubCL( csName, mainCLName, subCLNames, shardRanges );
   checkRecycleItem( recycleName );

   // 创建CL挂载到主表，在丢失的数据源子表范围内
   commCreateCL( db, csName, clName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );

   // 插入数据并校验
   var docs = [];
   for( var i = 0; i < 1500; i++ )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs.push( { a: i, b: bValue, c: cValue } );
   }
   mainCL.insert( docs );
   var cursor = mainCL.find().sort( { a: 1 } );
   docs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, docs );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   cleanRecycleBin( db, csName );
   if( dsVersion[0] >= 3 && dsVersion[1] >= 6 )
   {
      cleanRecycleBin( datasrcDB, srcCSName );
   }
   datasrcDB.close();
}