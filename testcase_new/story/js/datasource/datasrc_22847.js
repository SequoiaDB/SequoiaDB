/******************************************************************************
 * @Description   : seqDB-22847 :: 修改数据源名称
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc22847";
   var dataSrcName1 = "数据源datasrc22847";
   var dataSrcName2 = "datasrc22847C";
   var csName = "cs_22847";
   var csName1 = "cs_22847c";
   var srcCSName = "datasrcCS_22847";
   var clName = "cl_22847";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   clearDataSource( csName1, dataSrcName1 );
   clearDataSource( csName1, dataSrcName2 );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 } } );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   db.createDataSource( dataSrcName2, datasrcUrl, userName, passwd );
   db.createCS( csName1, { DataSource: dataSrcName2, Mapping: srcCSName } );

   //test a: 修改为其它名称   
   alterDataSourceAndCheckResult( dataSrcName, dataSrcName1, csName, clName );

   //test b: 修改为同名   
   alterDataSourceAndCheckResult( dataSrcName1, dataSrcName1, csName, clName );

   //test c: 修改为已删除的数据源名    
   db.dropCS( csName );
   db.dropDataSource( dataSrcName1 );
   alterDataSourceAndCheckResult( dataSrcName2, dataSrcName1, csName1, clName );

   datasrcDB.dropCS( srcCSName );
   db.dropCS( csName1 );
   db.dropDataSource( dataSrcName1 );
   datasrcDB.close();
}

function alterDataSourceAndCheckResult ( oldName, newName, csName, clName )
{
   var dataSourceObj = db.getDataSource( oldName );
   dataSourceObj.alter( { Name: newName } );

   var newdataSourceObj = db.getDataSource( newName );
   assert.equal( newName, newdataSourceObj._name );

   var dbcl = db.getCS( csName ).getCL( clName );
   var docs = [{ a: 1, b: 1, c: "testc" }, { a: 2, b: [1, "test"], c: "testsgasdgasdg" }, { a: 3, b: 234.3, c: { a: 1 } }, { a: 4, b: 4, c: "testtess4" }];
   dbcl.insert( docs );
   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   dbcl.remove();
}
