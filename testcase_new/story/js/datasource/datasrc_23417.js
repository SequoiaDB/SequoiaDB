/******************************************************************************
 * @Description   : seqDB-23417:源集群上修改集合空间/集合属性
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.02.04
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23417";
   var csName = "cs_23417";
   var srcCSName = "datasrcCS_23417";
   var clName = "cl_23417";
   var domainName = "domain_23417";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   commCreateDomain( db, domainName );

   assert.tryThrow( [SDB_OPERATION_INCOMPATIBLE], function()
   {
      db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName, PageSize: 65536 } );
   } );

   assert.tryThrow( [SDB_OPERATION_INCOMPATIBLE], function()
   {
      db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName, Domain: domainName } );
   } );

   assert.tryThrow( [SDB_OPERATION_INCOMPATIBLE], function()
   {
      db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName, LobPageSize: 65536 } );
   } );

   commDropDomain( db, domainName );
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}