/******************************************************************************
 * @Description   : seqDB-23298:源集群上修改集合空间/集合属性
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23298";
   var csName = "cs_23298";
   var srcCSName = "datasrcCS_23298";
   var clName = "cl_23298";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "sequoiadb", {ErrorControlLevel: "high"} );
   var dbcs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );

   assert.tryThrow( [SDB_OPERATION_INCOMPATIBLE], function()
   {
      dbcs.alter( { "LobPageSize": 8192 } );
   } );

   commDropCS( db, csName );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   assert.tryThrow( [SDB_OPERATION_INCOMPATIBLE], function()
   {
      dbcs.alter( { "LobPageSize": 8192 } );
   } );

   assert.tryThrow( [SDB_OPERATION_INCOMPATIBLE], function()
   {
      dbcl.alter( { "ReplSize": -1 } );
   } );

   var expectResult = [{ ReplSize: "" }];
   var cursor = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + clName }, { ReplSize: "" } );
   commCompareResults( cursor, expectResult );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}