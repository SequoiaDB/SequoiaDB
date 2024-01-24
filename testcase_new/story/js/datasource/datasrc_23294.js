/******************************************************************************
 * @Description   : seqDB-23294:内置SQL中list查看数据源列表 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.02.04
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName1 = "datasrc23294_1";
   var dataSrcName2 = "datasrc23294_2";
   var dataSrcName3 = "datasrc23294_3";

   clearDataSource( "nocs", dataSrcName1 );
   clearDataSource( "nocs", dataSrcName2 );
   clearDataSource( "nocs", dataSrcName3 );

   db.createDataSource( dataSrcName1, datasrcUrl, userName, passwd );
   db.createDataSource( dataSrcName2, datasrcUrl, userName, passwd );
   db.createDataSource( dataSrcName3, datasrcUrl, userName, passwd );

   var command = "select Name from $LIST_DATASOURCE where Name like 'datasrc23294_1|datasrc23294_2|datasrc23294_3' order by Name desc";

   var cursor = db.exec( command );
   var expectedResult = [{ Name: "datasrc23294_3" },
   { Name: "datasrc23294_2" },
   { Name: "datasrc23294_1" }]
   commCompareResults( cursor, expectedResult );

   clearDataSource( "nocs", dataSrcName1 );
   clearDataSource( "nocs", dataSrcName2 );
   clearDataSource( "nocs", dataSrcName3 );
}