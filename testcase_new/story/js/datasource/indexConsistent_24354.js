/******************************************************************************
 * @Description   : seqDB-24354:使用数据源创建集合，集合上创建/删除索引 
 * @Author        : liuli
 * @CreateTime    : 2021.09.17
 * @LastEditTime  : 2021.09.17
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var dataSrcName = "datasrc_24354";
   var csName = "cs_24354";
   var srcCSName = "datasrcCS_24354";
   var clName = "cl_24354";
   var indexName1 = "index_24354_1";
   var indexName2 = "index_24354_2";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   // 本地创建CL映射数据源上的集合
   var srccl = commCreateCL( datasrcDB, srcCSName, clName );
   var dbcl = commCreateCL( db, csName, clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   insertBulkData( dbcl, 2000 );

   // srccl上创建索引indexName1
   srccl.createIndex( indexName1, { a: 1 } );

   // 本地cl上创建索引indexName2
   dbcl.createIndex( indexName2, { c: 1 } );

   // 检查不存在创建索引任务
   checkNoTask( csName, clName, "Create index" );
   checkIndexExist( datasrcDB, srcCSName, clName, indexName2, false );

   // 本地cl上删除索引indexName1
   dbcl.dropIndex( indexName1 );

   // 检查不存在删除索引任务
   checkNoTask( csName, clName, "Drop index" );
   checkIndexExist( datasrcDB, srcCSName, clName, indexName1, true );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}