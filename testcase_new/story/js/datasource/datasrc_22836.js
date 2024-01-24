/******************************************************************************
 * @Description   : seqDB-22836:创建数据源指定地址中包含源集群节点地址
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.02.06
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22836";
   clearDataSource( "nocs", dataSrcName );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createDataSource( dataSrcName, datasrcUrl + "," + COORDHOSTNAME + ":" + COORDSVCNAME, userName, passwd );
   } );

   assert.tryThrow( SDB_CAT_DATASOURCE_NOTEXIST, function()
   {
      db.getDataSource( dataSrcName );
   } );
}


