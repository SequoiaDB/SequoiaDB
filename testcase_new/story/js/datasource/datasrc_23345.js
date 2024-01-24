/******************************************************************************
 * @Description   : seqDB-23345:创建使用数据源的集合，连接不同会话删除集合再创建本地同名集合执行数据操作
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{

   var db1 = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var dataSrcName = "datasrc23345";
   var csName = "cs_23345";
   var clName = "cl_23345";
   var srcCSName = "datasrcCS_23345";
   var docs = [{ a: 1 }];

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   var dataSrcCL = commCreateCL( datasrcDB, srcCSName, clName );
   var dbcs = commCreateCS( db1, csName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   // 删除集合后创建同名不使用数据源的集合
   db1.getCS( csName ).dropCL( clName );
   var dbcl2 = db1.getCS( csName ).createCL( clName );

   // 连接1执行插入数据
   dbcl.insert( docs );

   // 连接2执行数据操作
   dbcl2.insert( docs );
   var cursor = dataSrcCL.find();
   commCompareResults( cursor, [] );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
   db1.close();
}