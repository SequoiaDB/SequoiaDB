/******************************************************************************
 * @Description   : seqDB-22864:指定字段排序查看数据源
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.02.04
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName1 = "datasrc22864_1";
   var dataSrcName2 = "datasrc22864_2";
   var dataSrcName3 = "datasrc22864_3";
   var dataSrcName4 = "datasrc22864_4";
   var dataSrcName5 = "datasrc22864_5";

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

   var cursor = db.listDataSources( { Name: { $regex: "datasrc22864_*" } }, { Name: { $include: 1 } }, { Name: -1 } );
   var expectedResult = [{ Name: "datasrc22864_5" },
   { Name: "datasrc22864_4" },
   { Name: "datasrc22864_3" },
   { Name: "datasrc22864_2" },
   { Name: "datasrc22864_1" }];
   commCompareResults( cursor, expectedResult );

   clearDataSource( "nocs", dataSrcName1 );
   clearDataSource( "nocs", dataSrcName2 );
   clearDataSource( "nocs", dataSrcName3 );
   clearDataSource( "nocs", dataSrcName4 );
   clearDataSource( "nocs", dataSrcName5 );
}