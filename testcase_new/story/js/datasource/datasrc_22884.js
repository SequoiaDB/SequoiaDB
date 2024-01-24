/******************************************************************************
 * @Description   : seqDB-22884:在使用数据源的集合空间下创建使用数据源的集合
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22884";
   var csName = "cs_22884";
   var clName = "cl_22884";
   var srcCSName = "datasrcCS_22884";
   var datasrcDB = new Sdb( datasrcIp, datasrcPort, userName, passwd );
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   var cs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      cs.createCL( clName );
   } );

   clearDataSource( csName, dataSrcName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
}


