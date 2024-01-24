/******************************************************************************
 * @Description   : seqDB-22849:修改数据源地址，其中更新地址不可用
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22849";
   var csName = "cs_22849";
   var clName = "cl_22849";
   var srcCSName = "datasrcCS_22849";
   var coordUrls = getCoordUrl( db );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var dataSource = db.getDataSource( dataSrcName );

   assert.tryThrow( [SDB_INVALIDARG], function()
   {
      dataSource.alter( { Address: datasrcUrl + "," + coordUrls[0] } );
   } );
   var actualReasult = db.listDataSources( { "Name": dataSrcName } ).current().toObj().Address;
   assert.equal( actualReasult, datasrcUrl );

   // 校验新增其他数据源地址，需要使用两个数据源CI屏蔽该部分
   // assert.tryThrow( [SDB_INVALIDARG], function()
   // {
   //    dataSource.alter( { Address: datasrcUrl + "," + otherDSUrl1 } );
   // } );
   // var actualReasult = db.listDataSources( { "Name": dataSrcName } ).current().toObj().Address;
   // assert.equal( actualReasult, datasrcUrl );

   var docs = [{ a: 1, b: 1, c: "testc" }, { a: 2, b: [1, "test"], c: "testsgasdgasdg" }, { a: 3, b: 234.3, c: { a: 1 } }, { a: 4, b: 4, c: "testtess4" }];
   dbcl.insert( docs );
   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   commCompareResults( cursor, docs );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}