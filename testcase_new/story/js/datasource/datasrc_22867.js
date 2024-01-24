/******************************************************************************
 * @Description   : seqDB-22867:数据源名称为更新数据源名称，获取数据源信息
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.02.04
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName1 = "datasrc22867_a";
   var dataSrcName2 = "datasrc22867_b";
   clearDataSource( "nocs", dataSrcName1 );
   clearDataSource( "nocs", dataSrcName2 );

   db.createDataSource( dataSrcName1, datasrcUrl, userName, passwd );
   var dataSource = db.getDataSource( dataSrcName1 );
   dataSource.alter( { Name: dataSrcName2 } );

   db.createDataSource( dataSrcName1, datasrcUrl, userName, passwd, "SequoiaDB", { AccessMode: "READ", ErrorFilterMask: "WRITE" } );
   var explainObj = db.listDataSources( { Name: dataSrcName1 } );
   checkExplain( explainObj, dataSrcName1, datasrcUrl, "sdbadmin", "READ", "WRITE", "low" );

   clearDataSource( "nocs", dataSrcName1 );
   clearDataSource( "nocs", dataSrcName2 );
}

function checkExplain ( explainObj, name, address, user, accessModeDesc, errorFilterMaskDesc, errorControlLevel )
{
   assert.equal( explainObj.current().toObj().Name, name );
   assert.equal( explainObj.current().toObj().Address, address );
   // assert.equal( explainObj.current().toObj().User, user );
   assert.equal( explainObj.current().toObj().AccessModeDesc, accessModeDesc );
   assert.equal( explainObj.current().toObj().ErrorFilterMaskDesc, errorFilterMaskDesc );
   assert.equal( explainObj.current().toObj().ErrorControlLevel, errorControlLevel );
   explainObj.close();
}