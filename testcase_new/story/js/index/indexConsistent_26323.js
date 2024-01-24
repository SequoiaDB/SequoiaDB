/******************************************************************************
 * @Description   : seqDB-26323:异步复制索引后立即创建/删除索引 
 * @Author        : liuli
 * @CreateTime    : 2022.04.02
 * @LastEditTime  : 2022.04.07
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_26323";
   var mainCLName = "maincl_26323";
   var subCLName1 = "subcl_26323_1";
   var subCLName2 = "subcl_26323_2";
   var indexName1 = "index_26323_1";
   var indexName2 = "index_26323_2";
   var recordNum = 1000;

   commDropCS( db, csName );
   var dbcs = commCreateCS( db, csName );
   var maincl = dbcs.createCL( mainCLName, { IsMainCL: true, ShardingKey: { a: 1 } } );
   var subcl1 = dbcs.createCL( subCLName1, { ShardingKey: { a: 1 } } );
   dbcs.createCL( subCLName2, { ShardingKey: { a: 1 } } );

   // 主表创建索引
   maincl.createIndex( indexName1, { b: 1 } );
   maincl.createIndex( indexName2, { c: 1 } );

   // 挂载子表
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 500 }, UpBound: { a: 1000 } } );
   insertBulkData( maincl, recordNum );

   // 主表异步复制索引
   var taskId = maincl.copyIndexAsync( csName + "." + subCLName1, indexName1 );

   // 主表删除索引
   try
   {
      maincl.dropIndex( indexName1 );
   }
   catch( e )
   {
      if( e != SDB_IXM_CREATING )
      {
         throw e;
      }
   }
   db.waitTasks( taskId );

   // 主表异步复制索引
   var taskId = maincl.copyIndexAsync( csName + "." + subCLName1, indexName2 );

   // 子表删除索引
   try
   {
      subcl1.dropIndex( indexName2 );
   }
   catch( e )
   {
      if( e != SDB_IXM_NOTEXIST && e != SDB_IXM_CREATING )
      {
         throw e;
      }
   }
   db.waitTasks( taskId );
   commDropCS( db, csName );
}
