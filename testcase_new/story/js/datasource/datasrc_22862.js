/******************************************************************************
 * @Description   : seqDB-22862:指定查询条件查看数据源 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.02.04
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName1 = "datasrc22862_a";
   var dataSrcName2 = "datasrc22862_b";
   clearDataSource( "nocs", dataSrcName1 );
   clearDataSource( "nocs", dataSrcName2 );
   db.createDataSource( dataSrcName1, datasrcUrl, userName, passwd );
   db.createDataSource( dataSrcName2, datasrcUrl, userName, passwd );

   var explainObj = db.listDataSources( { Name: dataSrcName1 } );
   checkExplain( explainObj, dataSrcName1, datasrcUrl, userName );

   var explainObj = db.listDataSources( { Group: dataSrcName1 } );
   commCompareResults( explainObj, [] );

   clearDataSource( "nocs", dataSrcName1 );
   clearDataSource( "nocs", dataSrcName2 );
}

function checkExplain ( explainObj, name, address, user )
{
   assert.equal( explainObj.current().toObj().Name, name );
   assert.equal( explainObj.current().toObj().Address, address );
   assert.equal( explainObj.current().toObj().User, user );
   explainObj.close();
}