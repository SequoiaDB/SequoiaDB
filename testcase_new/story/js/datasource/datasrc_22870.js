/******************************************************************************
 * @Description   : seqDB-22870:使用数据源创建cs，数据源上不存在该cs
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var csName = "cs_22870";
   var dataSrcName = "datasrc22870";
   clearDataSource( csName, dataSrcName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   //a、指定Mapping参数映射同名cs，cs不存在
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.createCS( csName, { DataSource: dataSrcName, Mapping: csName } );
   } );

   //b、指定Mapping参数映射不同名cs，cs不存在   
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.createCS( csName, { DataSource: dataSrcName, Mapping: "noexistCS" } );
   } );

   //c、不指定Mapping参数，其中同名cs在数据源不存在
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.createCS( csName, { DataSource: dataSrcName } );
   } );

   db.dropDataSource( dataSrcName );
}
