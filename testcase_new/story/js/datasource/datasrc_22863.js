/******************************************************************************
 * @Description   : seqDB-22863:指定查询条件查看数据源 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.02.04
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName1 = "datasrc22863_1";
   var dataSrcName2 = "datasrc22863_2";
   var dataSrcName3 = "datasrc22863_3";
   var dataSrcName4 = "datasrc22863_4";
   clearDataSource( "nocs", dataSrcName1 );
   clearDataSource( "nocs", dataSrcName2 );
   clearDataSource( "nocs", dataSrcName3 );
   clearDataSource( "nocs", dataSrcName4 );

   db.createDataSource( dataSrcName1, datasrcUrl, userName, passwd, "SequoiaDB", { "AccessMode": "NONE" } );
   db.createDataSource( dataSrcName2, datasrcUrl, userName, passwd, "SequoiaDB", { "AccessMode": "READ" } );
   db.createDataSource( dataSrcName3, datasrcUrl, userName, passwd, "SequoiaDB", { "AccessMode": "WRITE" } );
   db.createDataSource( dataSrcName4, datasrcUrl, userName, passwd, "SequoiaDB", { "AccessMode": "ALL" } );
   var explainObj = db.listDataSources( { Name: dataSrcName2 }, { AccessModeDesc: { $include: 1 } } ).current().toObj().AccessModeDesc;
   assert.equal( explainObj, "READ" );
   var explainObj = db.listDataSources( { Name: dataSrcName3 }, { AccessModeDesc: { $include: 1 } } ).current().toObj().AccessModeDesc;
   assert.equal( explainObj, "WRITE" );

   clearDataSource( "nocs", dataSrcName1 );
   clearDataSource( "nocs", dataSrcName2 );
   clearDataSource( "nocs", dataSrcName3 );
   clearDataSource( "nocs", dataSrcName4 );
}
