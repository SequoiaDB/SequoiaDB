/******************************************************************************
 * @Description   : seqDB-22846 ::删除正在使用的数据源 
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc22846";
   var csName = "cs_22846";
   var srcCSName = "datasrcCS_22846";
   var clName = "cl_22846";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 } } );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var cs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );

   assert.tryThrow( SDB_CAT_DATASOURCE_INUSE, function()
   {
      db.dropDataSource( dataSrcName );
   } );

   clearDataSource( csName, dataSrcName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();

}
