/******************************************************************************
 * @Description   : seqDB-24201:修改TransPropagateMode配置，事务中使用数据源
 * @Author        : liuli
 * @CreateTime    : 2021.05.27
 * @LastEditTime  : 2021.05.30
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var dataSrcName = "datasrc24201";
   var csName = "cs_24201";
   var clName = "cl_24201";
   var srcCSName = "datasrcCS_24201";
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test" }];

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   var src = db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { "TransPropagateMode": "notsupport" } );

   var dbcl = db.createCS( csName ).createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   // 修改TransPropagateMode为never
   src.alter( { "TransPropagateMode": "never" } );
   var explainObj = db.listDataSources( { Name: dataSrcName } );
   assert.equal( explainObj.current().toObj().TransPropagateMode, "never" );
   explainObj.close();

   // 开启事务插入数据
   db.transBegin();
   assert.tryThrow( [SDB_COORD_DATASOURCE_TRANS_FORBIDDEN], function()
   {
      dbcl.insert( docs );
   } );

   // 修改TransPropagateMode为notsupport
   src.alter( { "TransPropagateMode": "notsupport" } );
   var explainObj = db.listDataSources( { Name: dataSrcName } );
   assert.equal( explainObj.current().toObj().TransPropagateMode, "notsupport" );
   explainObj.close();

   // 开启事务插入数据
   db.transBegin();
   dbcl.insert( docs );
   db.transCommit();

   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}