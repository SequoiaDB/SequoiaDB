/******************************************************************************
 * @Description   : seqDB-22848 :: 修改数据源地址为可用地址
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc22848";
   var csName = "cs_22848";
   var srcCSName = "datasrcCS_22848";
   var clName = "cl_22848";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 } } );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );


   //test a: 新增一个有效地址，数据源上其它节点连接地址   
   var datasrcUrls = getCoordUrl( datasrcDB );
   alterDataSourceAndCheckResult( dataSrcName, datasrcUrls.toString(), csName, clName );
   //test b: 新增一个不可连接地址   
   //alterDataSourceAndCheckResult(dataSrcName, datasrcUrl + "," + datasrcIp + ":11530", csName, clName);   
   //test c: 新增重复地址    
   alterDataSourceAndCheckResult( dataSrcName, datasrcUrl + "," + datasrcUrl, csName, clName );
   //test d: 修改为1个可用地址
   alterDataSourceAndCheckResult( dataSrcName, datasrcUrl, csName, clName );

   clearDataSource( csName, dataSrcName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
}

function alterDataSourceAndCheckResult ( name, address, csName, clName )
{
   var dataSourceObj = db.getDataSource( name );
   dataSourceObj.alter( { "Address": address } );

   var dbcl = db.getCS( csName ).getCL( clName );
   var docs = [{ a: 1, b: 1, c: "testc" }, { a: 2, b: [1, "test"], c: "testsgasdgasdg" }, { a: 3, b: 234.3, c: { a: 1 } }, { a: 4, b: 4, c: "testtess4" }];
   dbcl.insert( docs );

   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   dbcl.remove();
}
