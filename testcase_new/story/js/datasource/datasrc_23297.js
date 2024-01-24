/******************************************************************************
 * @Description   : seqDB-23297:执行list指定条件查询数据源 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.02.04
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName1 = "datasrc23297_1";
   var dataSrcName2 = "datasrc23297_2";
   var dataSrcName3 = "datasrc23297_3";
   var dataSrcName4 = "datasrc23297_4";
   var dataSrcName5 = "datasrc23297_5";

   clearDataSource( "nocs", dataSrcName1 );
   clearDataSource( "nocs", dataSrcName2 );
   clearDataSource( "nocs", dataSrcName3 );
   clearDataSource( "nocs", dataSrcName4 );
   clearDataSource( "nocs", dataSrcName5 );

   db.createDataSource( dataSrcName1, datasrcUrl, userName, passwd );
   db.createDataSource( dataSrcName2, datasrcUrl, userName, passwd );
   db.createDataSource( dataSrcName3, datasrcUrl, userName, passwd );
   db.createDataSource( dataSrcName4, datasrcUrl, userName, passwd );
   db.createDataSource( dataSrcName5, datasrcUrl, userName, passwd );

   var expectResult = [];
   var cursor = db.list( SDB_LIST_DATASOURCES, { Name: { $regex: dataSrcName1 } } );
   var actualResults = db.listDataSources( { Name: dataSrcName1 } ).current().toObj();
   expectResult.push( actualResults );
   commCompareResults( cursor, expectResult, false );

   var expectResult = [];
   var cursor = db.list( SDB_LIST_DATASOURCES, { Name: { $regex: dataSrcName1 } }, {}, { Name: -1 } );
   var actualResults = db.listDataSources( { Name: dataSrcName1 }, {}, { Name: -1 } ).current().toObj();
   expectResult.push( actualResults );
   commCompareResults( cursor, expectResult, false );

   var cursor = db.list( SDB_LIST_DATASOURCES, { Name: { $regex: "datasrc23297_*" } }, { Name: { $include: 1 } }, { Name: -1 } );
   var expectedResult = [{ Name: "datasrc23297_5" },
   { Name: "datasrc23297_4" },
   { Name: "datasrc23297_3" },
   { Name: "datasrc23297_2" },
   { Name: "datasrc23297_1" }]
   commCompareResults( cursor, expectedResult );

   clearDataSource( "nocs", dataSrcName1 );
   clearDataSource( "nocs", dataSrcName2 );
   clearDataSource( "nocs", dataSrcName3 );
   clearDataSource( "nocs", dataSrcName4 );
   clearDataSource( "nocs", dataSrcName5 );
}