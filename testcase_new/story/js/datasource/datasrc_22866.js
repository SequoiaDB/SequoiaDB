/******************************************************************************
 * @Description   : seqDB-22866:数据源不存在，获取数据源 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.02.04
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22866";
   clearDataSource( "nocs", dataSrcName );

   assert.tryThrow( [SDB_CAT_DATASOURCE_NOTEXIST], function() 
   {
      db.getDataSource( dataSrcName );
   } );
}