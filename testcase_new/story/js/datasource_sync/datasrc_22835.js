/******************************************************************************
 * @Description   :seqDB-22835 :: 创建数据源指定地址中包含异常节点地址 
 * @Author        : Wu Yan
 * @CreateTime    : 2021.02.06
 * @LastEditTime  : 2021.03.16
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
//目前CI环境不支持运行该用例，暂时屏蔽
//main( test );
function test ()
{
   var dataSrcName = "datasrc22835";
   var csName = "cs_22835";
   var clName = "cl_22835";
   var srcCSName = "datasrcCS_22835";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );

   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName, { ShardingKey: { a: 1 } } );
   //192.168.30.110:11910为异常地址
   db.createDataSource( dataSrcName, datasrcUrl + ", 192.168.30.110:11910", userName, passwd );

   var cs = db.createCS( csName );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   var docs = [{ a: 1, b: 1, c: "testc" }, { a: 2, b: [1, "test"], c: "testsgasdgasdg" }, { a: 3, b: 234.3, c: { a: 1 } }, { a: 4, b: 4, c: "testtess4" }];
   dbcl.insert( docs );
   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   commCompareResults( cursor, docs );

   clearDataSource( csName, dataSrcName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
}


