/******************************************************************************
 * @Description   : seqDB-22843:创建数据源，设置过滤数据读写错误
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc22843";
   var csName = "cs_22843";
   var clName = "cl_22843";
   var srcCSName = "datasrcCS_22843";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { "ErrorFilterMask": "ALL" } );

   // CS 级别映射
   var dbcs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = dbcs.getCL( clName );
   commDropCS( datasrcDB, srcCSName );
   allAndCheckResult( dbcl );

   // CL 级别映射
   commDropCS( db, csName );
   commCreateCL( datasrcDB, srcCSName, clName );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   commDropCS( datasrcDB, srcCSName );
   allAndCheckResult( dbcl );

   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function allAndCheckResult ( dbcl )
{
   dbcl.insert( { a: 1 } );
   dbcl.remove();
   dbcl.truncate();
   dbcl.find().toArray();
   dbcl.upsert( { $set: { b: 1 } }, { a: 1 } );
   dbcl.find( { a: 1 } ).remove().toArray();
   dbcl.find( { a: 1 } ).update( { $set: { a: 3 } } ).toArray();
}