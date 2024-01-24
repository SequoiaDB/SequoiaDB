/******************************************************************************
 * @Description   : seqDB-23378:使用数据源创建cl，映射集合创建索引
 *                : seqDB-24423:创建CL并使用CS.listCollections() (测试集合使用数据源)
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.10.14
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23378";
   var csName = "cs_23378";
   var clName = "cl_23378";
   var srcCSName = "datasrcCS_23378";
   var indexName = "index_23378";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "sequoiadb", { ErrorControlLevel: "high" } );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   // 测试seqDB-24423:创建CL并使用CS.listCollections() (测试集合使用数据源)
   var cursor = dbcs.listCollections();
   var clNum = 0;
   while( cursor.next() )
   {
      assert.equal( cursor.current().toObj()["Name"], csName + "." + clName );
      clNum++;
   }
   cursor.close();
   assert.equal( clNum, 1 );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.createIndex( indexName, { a: 1 }, true, true );
   } );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
}
