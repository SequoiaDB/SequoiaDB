/******************************************************************************
 * @Description   : seqDB-22845:创建多个数据源指定相同集群
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.02.04
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName1 = "datasrc22845_1";
   var dataSrcName2 = "datasrc22845_2";

   clearDataSource( "nocs", dataSrcName1 );
   clearDataSource( "nocs", dataSrcName2 );

   db.createDataSource( dataSrcName1, datasrcUrl, userName, passwd );
   db.createDataSource( dataSrcName2, datasrcUrl, userName, passwd );

   var expectResult = [];
   var cursor = db.list( SDB_LIST_DATASOURCES, { Name: dataSrcName1 } );
   var actualResults = db.listDataSources( { Name: dataSrcName1 } ).current().toObj();
   expectResult.push( actualResults );
   commCompareResults( cursor, expectResult, false );

   clearDataSource( "nocs", dataSrcName1 );
   clearDataSource( "nocs", dataSrcName2 );
}