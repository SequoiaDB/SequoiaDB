/******************************************************************************
 * @Description   : seqDB-22832:创建/删除数据源
 *                  seqDB-22833:创建同名数据源
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.02.06
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22832";
   clearDataSource( "nocs", dataSrcName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var obj = db.getDataSource( dataSrcName );

   if( dataSrcName != obj._name )
   {
      throw new Error( " act datasrc name is " + obj._name );
   }

   //seqDB-22833:创建同名数据源
   assert.tryThrow( SDB_CAT_DATASOURCE_EXIST, function()
   {
      db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   } );

   db.dropDataSource( dataSrcName );
   assert.tryThrow( SDB_CAT_DATASOURCE_NOTEXIST, function()
   {
      db.getDataSource( dataSrcName );
   } );
}
