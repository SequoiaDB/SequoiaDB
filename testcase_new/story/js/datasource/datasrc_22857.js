/******************************************************************************
 * @Description   : seqDB-22857:修改数据源多个属性
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22857";
   var csName = "cs_22857";
   var clName = "cl_22857";
   var srcCSName = "datasrcCS_22857";
   var indexName = "index_22857";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dataSource = db.getDataSource( dataSrcName );
   var dbcs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = dbcs.getCL( clName );

   dataSource.alter( { "AccessMode": "READ", "ErrorFilterMask": "READ", "ErrorControlLevel": "Low" } );
   var explainObj = db.listDataSources( { "Name": dataSrcName } );
   checkExplain( explainObj, dataSrcName, datasrcUrl, userName, "READ", "READ", "low" );

   // 创建索引，验证 "ErrorControlLevel": "low"
   dbcl.createIndex( indexName, { a: 1 } );

   // 插入数据，验证 "AccessMode": "READ"
   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function()
   {
      dbcl.insert( { a: 1 } );
   } );

   // 删除数据源上的CS后查询数据，验证 "ErrorFilterMask": "READ"
   commDropCS( datasrcDB, srcCSName );
   dbcl.find().toArray();

   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function checkExplain ( explainObj, name, address, user, accessModeDesc, errorFilterMaskDesc, errorControlLevel )
{
   assert.equal( explainObj.current().toObj().Name, name );
   assert.equal( explainObj.current().toObj().Address, address );
   assert.equal( explainObj.current().toObj().User, user );
   assert.equal( explainObj.current().toObj().AccessModeDesc, accessModeDesc );
   assert.equal( explainObj.current().toObj().ErrorFilterMaskDesc, errorFilterMaskDesc );
   assert.equal( explainObj.current().toObj().ErrorControlLevel, errorControlLevel );
   explainObj.close();
}