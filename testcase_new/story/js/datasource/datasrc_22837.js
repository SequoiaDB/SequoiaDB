/******************************************************************************
 * @Description   : seqDB-22837 :: 创建数据源指定多个数据源的节点地址
 * @Author        : Wu Yan
 * @CreateTime    : 2021.10.20
 * @LastEditTime  : 2021.03.16
 * @LastEditors   : Wu Yan
 ******************************************************************************/
//目前CI上没有多个数据源的环境，暂时屏蔽
//main( test );
function test ()
{
   var dataSrcName = "datasrc22837";

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createDataSource( dataSrcName, datasrcUrl + "," + datasrcUrl1, userName, passwd );
   } );

   assert.tryThrow( SDB_CAT_DATASOURCE_NOTEXIST, function()
   {
      db.getDataSource( dataSrcName );
   } );
}


