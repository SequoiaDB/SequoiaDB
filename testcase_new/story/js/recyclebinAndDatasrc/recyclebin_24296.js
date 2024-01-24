/******************************************************************************
 * @Description   : seqDB-24296:一个子表使用数据源，从使用数据源的子表执行truncate
 * @Author        : liuli
 * @CreateTime    : 2021.07.23
 * @LastEditTime  : 2022.08.15
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc24296";
   var csName = "cs_24296";
   var clName = "cl_24296";
   var srcCSName = "datasrcCS_24296";
   var mainCLName = "mainCL_24296";
   var subCLName = "subCL_24296";

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

   // 插入数据在使用数据表的子表
   var docs = [];
   var docs1 = [];
   for( var i = 0; i < 800; i++ )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs1.push( { a: i, b: bValue, c: cValue } );
      docs.push( { a: i, b: bValue, c: cValue } );
   }
   mainCL.insert( docs1 );

   // 插入数据在本地子表
   var docs2 = [];
   for( var i = 1200; i < 1800; i++ )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs2.push( { a: i, b: bValue, c: cValue } );
      docs.push( { a: i, b: bValue, c: cValue } );
   }
   mainCL.insert( docs2 );

   // 使用数据源的子表执行truncate
   dbcl.truncate();
   var option = { "OriginName": srcCSName + "." + clName, "OpType": "Truncate" };
   var recyclebinItem = db.getRecycleBin().count( option );
   assert.equal( recyclebinItem, 0, "Recycle bin property error" );

   // 从主表校验数据
   var cursor = mainCL.find().sort( { a: 1 } );
   docs2.sort( sortBy( 'a' ) );
   commCompareResults( cursor, docs2 );

   // 数据源端恢复truncate项目
   if( dsVersion[0] >= 3 && dsVersion[1] >= 6 && isRecycleBinOpen( datasrcDB ) )
   {
      var srcOriginName = srcCSName + "." + clName;
      var recycleName = getOneRecycleName( datasrcDB, srcOriginName, "Truncate" );
      datasrcDB.getRecycleBin().returnItem( recycleName );

      var cursor = mainCL.find().sort( { a: 1 } );
      docs.sort( sortBy( 'a' ) );
      commCompareResults( cursor, docs );
   }

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   cleanRecycleBin( db, csName );
   if( dsVersion[0] >= 3 && dsVersion[1] >= 6 )
   {
      cleanRecycleBin( datasrcDB, srcCSName );
   }
   datasrcDB.close();
}