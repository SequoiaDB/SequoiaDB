/******************************************************************************
 * @Description   : seqDB-22860:不同会话创建/修改数据源
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var dataSrcName = "datasrc22860";
   var csName = "cs_22860";
   var clName = "cl_22860";
   var srcCSName = "datasrcCS_22860";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   var dataSource = db1.getDataSource( dataSrcName );
   dataSource.alter( { "AccessMode": "READ" } );
   var actualResult = db.listDataSources( { "Name": dataSrcName } ).current().toObj().AccessModeDesc;
   assert.equal( actualResult, "READ" );

   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      dbcl.insert( { a: 1 } );
   } );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
   db1.close();
}