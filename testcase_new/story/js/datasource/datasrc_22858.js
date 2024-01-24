/******************************************************************************
 * @Description   : seqDB-22858:数据源正在使用，修改数据源属性
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22858";
   var csName = "cs_22858";
   var clName = "cl_22858";
   var srcCSName = "datasrcCS_22858";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dataSource = db.getDataSource( dataSrcName );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   dbcl.insert( { a: 1 } );
   dataSource.alter( { "AccessMode": "READ" } );
   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function()
   {
      dbcl.insert( { a: 1 } );
   } );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}