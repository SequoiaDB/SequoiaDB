/******************************************************************************
 * @Description   : seqDB-22887:数据源正在使用，源集群上删除使用数据源cl
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.16
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc22887";
   var csName = "cs_22887";
   var srcCSName = "datasrcCS_22887";
   var clName = "cl_22887";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var cs = db.createCS( csName );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   dbcl.insert( { a: "test" } );
   cs.dropCL( clName );

   //源集群上查询存在新插入记录{a:"test"}
   var count = datasrcDB.getCS( srcCSName ).getCL( clName ).count( { a: "test" } );
   assert.equal( count, 1 );

   clearDataSource( csName, dataSrcName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
}
