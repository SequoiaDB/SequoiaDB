/******************************************************************************
 * @Description   : seqDB-22851:修改数据源地址为不同数据源 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
// 用例需要2个数据源
// main( test );
function test ()
{

   var csName = "cs_22851";
   var clName = "cl_22851";
   var srcCSName = "datasrcCS_22851";
   var dataSrcName = "datasrc22851";
   var datasrcDB1 = new Sdb( other_datasrcIp1, datasrcPort, userName, passwd );

   commDropCS( datasrcDB1, srcCSName );
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB1, srcCSName, clName );

   db.createDataSource( dataSrcName, otherDSUrl1, userName, passwd );
   var dataSource = db.getDataSource( dataSrcName );
   var cs = db.createCS( csName );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   dataSource.alter( { "User": userName, "Password": passwd, "Address": datasrcUrl } );
   var explainObj = db.listDataSources( { "Name": dataSrcName } );
   checkExplain( explainObj, dataSrcName, datasrcUrl, userName );

   var docs = [{ a: 1, b: 1, c: "testc" }, { a: 2, b: [1, "test"], c: "testsgasdgasdg" }, { a: 3, b: 234.3, c: { a: 1 } }, { a: 4, b: 4, c: "testtess4" }];
   try
   {
      dbcl.insert( docs );
      throw new Error( "insert should be fail" );
   }
   catch( e )
   {
      if( e != SDB_DMS_NOTEXIST && e != SDB_CAT_DATASOURCE_NOTEXIST )
      {
         throw new Error( e );
      }
   }

   commDropCS( datasrcDB1, srcCSName );
   commDropCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName );
   dbcl.insert( docs );
   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   commCompareResults( cursor, docs );

   commDropCS( datasrcDB1, srcCSName );
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
   datasrcDB1.close();
}

function checkExplain ( explainObj, name, address, user, accessMode, errorFilterMask )
{
   assert.equal( explainObj.current().toObj().Name, name );
   assert.equal( explainObj.current().toObj().Address, address );
   assert.equal( explainObj.current().toObj().User, user );
   explainObj.close();
}