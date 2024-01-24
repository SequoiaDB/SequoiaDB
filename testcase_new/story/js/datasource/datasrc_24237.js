/******************************************************************************
 * @Description   : seqDB-24237:一个子表使用数据源，设置TransPropagateMode属性
 * @Author        : liuli
 * @CreateTime    : 2021.05.27
 * @LastEditTime  : 2021.05.27
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc24237";
   var csName = "cs_24237";
   var clName = "cl_24237";
   var srcCSName = "datasrcCS_24237";
   var mainCLName = "mainCL_24237";
   var subCLName = "subCL_24237";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   var src = db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB" );

   var dbcs = db.createCS( csName );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   // 开启事务插入数据，覆盖本地子表和数据源子表
   db.transBegin();
   var docs = [];
   for( var i = 800; i < 1200; i++ )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs.push( { a: i, b: bValue, c: cValue } );
   }
   assert.tryThrow( [SDB_COORD_DATASOURCE_TRANS_FORBIDDEN], function()
   {
      mainCL.insert( docs );
   } );
   var cursor = mainCL.find();
   commCompareResults( cursor, [] );

   // 开启事务插入数据，只在本地子表
   db.transBegin();
   var docs = [];
   for( var i = 0; i < 400; i++ )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs.push( { a: i, b: bValue, c: cValue } );
   }
   mainCL.insert( docs );
   db.transCommit();
   var cursor = mainCL.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   // 修改TransPropagateMode为notsupport
   src.alter( { TransPropagateMode: "notsupport" } );

   mainCL.remove();
   // 开启事务插入数据，覆盖本地子表和数据源子表
   db.transBegin();
   var docs = [];
   for( var i = 800; i < 1200; i++ )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs.push( { a: i, b: bValue, c: cValue } );
   }
   mainCL.insert( docs );

   // 提交事务，检查数据
   db.transCommit();
   var cursor = mainCL.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   mainCL.remove();
   // 开启事务插入数据，覆盖本地子表和数据源子表
   db.transBegin();
   var docs = [];
   var srcdocs = [];
   for( var i = 800; i < 1200; i++ )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs.push( { a: i, b: bValue, c: cValue } );
      if( i >= 1000 )
      {
         srcdocs.push( { a: i, b: bValue, c: cValue } );
      }
   }
   mainCL.insert( docs );

   // 回滚事务，检查数据
   db.transRollback();
   var cursor = mainCL.find().sort( { a: 1 } );
   commCompareResults( cursor, srcdocs );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}