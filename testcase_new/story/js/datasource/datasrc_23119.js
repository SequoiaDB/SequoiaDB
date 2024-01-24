/******************************************************************************
 * @Description   : seqDB-23119:创建数据源，指定地址包含主机名/IP地址
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23119";
   var csName = "cs_23119";
   var clName = "cl_23119";
   var srcCSName = "datasrcCS_23119";
   var coordArr = getCoordUrl( datasrcDB );
   if( coordArr.length < 2 )
   {
      return;
   }

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );

   // 主机名 + 端口
   db.createDataSource( dataSrcName, coordArr[0], userName, passwd );
   var explainObj = db.listDataSources( { "Name": dataSrcName } );
   checkExplain( explainObj, dataSrcName, coordArr[0], userName );
   var dbcs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = dbcs.getCL( clName );
   var docs = [{ a: 1, b: 1, c: "testc" }, { a: 2, b: [1, "test"], c: "testsgasdgasdg" }, { a: 3, b: 234.3, c: { a: 1 } }, { a: 4, b: 4, c: "testtess4" }];
   dbcl.insert( docs );
   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   dbcl.remove();
   clearDataSource( csName, dataSrcName );

   // 指定多个有效地址
   db.createDataSource( dataSrcName, coordArr[0] + "," + coordArr[1], userName, passwd );
   var explainObj = db.listDataSources( { "Name": dataSrcName } );
   checkExplain( explainObj, dataSrcName, coordArr[0] + "," + coordArr[1], userName );
   var dbcs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = dbcs.getCL( clName );
   var docs = [{ a: 1, b: 1, c: "testc" }, { a: 2, b: [1, "test"], c: "testsgasdgasdg" }, { a: 3, b: 234.3, c: { a: 1 } }, { a: 4, b: 4, c: "testtess4" }];
   dbcl.insert( docs );
   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   commCompareResults( cursor, docs );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function checkExplain ( explainObj, name, address, user, accessMode, errorFilterMask )
{
   if( typeof ( accessMode ) == "undefined" ) { accessMode = 3; }
   if( typeof ( errorFilterMask ) == "undefined" ) { errorFilterMask = 0; }
   assert.equal( explainObj.current().toObj().Name, name );
   assert.equal( explainObj.current().toObj().Address, address );
   assert.equal( explainObj.current().toObj().User, user );
   assert.equal( explainObj.current().toObj().AccessMode, accessMode );
   assert.equal( explainObj.current().toObj().ErrorFilterMask, errorFilterMask );
   explainObj.close();
}