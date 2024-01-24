/******************************************************************************
 * @Description   : seqDB-24376:集合连接数据源，创建本地索引
 * @Author        : liuli
 * @CreateTime    : 2021.09.27
 * @LastEditTime  : 2022.01.13
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var dataSrcName = "datasrc24376";
   var csName = "cs_24376";
   var srcCSName = "datasrcCS_24376";
   var clName = "cl_24376";
   var indexName = "index_24376";
   var groups = commGetDataGroupNames( db );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   var datasrc = db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   commCreateCL( datasrcDB, srcCSName, clName );
   var dbcl = commCreateCL( db, csName, clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   insertBulkData( dbcl, 2000 );

   // 随机获取一个节点
   var nodeName = getOneNodeName( db, groups[0] );
   dbcl.createIndex( indexName, { a: 1 }, { Standalone: true }, { NodeName: nodeName } );

   // 数据源集群上校验索引不存在
   checkStandaloneIndexOnNode( datasrcDB, srcCSName, clName, indexName, nodeName, false );

   // 修改数据源ErrorControlLevel为high
   datasrc.alter( { ErrorControlLevel: "high" } );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.createIndex( indexName, { a: 1 }, { Standalone: true }, { NodeName: nodeName } );
   } );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}