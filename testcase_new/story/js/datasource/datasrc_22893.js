/******************************************************************************
 * @Description   : seqDB-22893:使用数据源创建cs，关联集合创建索引
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22893";
   var csName = "cs_22893";
   var clName = "cl_22893";
   var srcCSName = "datasrcCS_22893";
   var indexName = "index_22893";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "sequoiadb", {ErrorControlLevel: "high"} );
   var dbcs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = dbcs.getCL( clName );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.createIndex( indexName, { a: 1 } );
   } );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}
