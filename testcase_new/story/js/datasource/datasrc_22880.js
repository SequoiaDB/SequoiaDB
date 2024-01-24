/******************************************************************************
 * @Description   : seqDB-22880:源集群上创建cl关联数据源上分区集合
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
// 分区表、切分表、主子表，执行数据操作
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22880";
   var csName = "cs_22880";
   var srcCSName = "datasrcCS_22880";
   var clName = "cl_22880";
   var mainCLName = "mainCL_22880";
   var subCLName = "subCL_22880";

   commDropCS( datasrcDB, srcCSName );
   commDropCS( db, srcCSName );
   clearDataSource( csName, dataSrcName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   // 主子表
   var mainCL = commCreateCL( datasrcDB, srcCSName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( datasrcDB, srcCSName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( srcCSName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( srcCSName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + mainCLName } );
   curdOperate( dbcl );

   // 分区表
   commDropCS( datasrcDB, srcCSName );
   commDropCS( db, csName );
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   curdOperate( dbcl );

   // 切分表
   commDropCS( datasrcDB, srcCSName );
   commDropCS( db, csName );
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 }, AutoSplit: true } );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   curdOperate( dbcl );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function curdOperate ( dbcl )
{
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];
   dbcl.insert( docs );
   dbcl.update( { $set: { b: 3 } }, { a: { $gt: 2 } } );
   dbcl.remove( { a: { $et: 2 } } );
   dbcl.insert( { a: 2, b: 2 } );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: 2 }, { a: 3, b: 3 }];
   var cursor = dbcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
}