/******************************************************************************
 * @Description   : seqDB-22844:创建/删除数据源指定所有属性值
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc22844";
   var csName = "cs_22844";
   var clName = "cl_22844";
   var srcCSName = "datasrcCS_22844";
   var indexName = "index_22844";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { "AccessMode": "READ", "ErrorFilterMask": "READ", "ErrorControlLevel": "Low" } );

   var explainObj = db.listDataSources( { "Name": dataSrcName } );
   checkExplain( explainObj, dataSrcName, datasrcUrl, userName, "READ", "READ", "low" );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   // 创建索引，验证 "ErrorControlLevel": "Low"
   dbcl.createIndex( indexName, { a: 1 } );

   // 插入数据，验证 "AccessMode": "READ"
   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function()
   {
      dbcl.insert( { a: 1 } );
   } );

   // 删除数据源上的CS后查询数据，验证 "ErrorFilterMask": "READ"
   commDropCS( datasrcDB, srcCSName );
   dbcl.find().toArray();

   // 删除数据源后使用 cl
   dbcs.dropCL( clName );
   db.dropDataSource( dataSrcName );

   assert.tryThrow( [SDB_DMS_NOTEXIST], function()
   {
      dbcl.insert( { a: 1 } );
   } );

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