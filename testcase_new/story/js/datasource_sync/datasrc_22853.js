/******************************************************************************
 * @Description   : seqDB-22853:修改数据源用户名和密码
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.04.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
// CI环境不支持运行数据源串行用例，暂时屏蔽
// try
// {
//    main( test );
// }
// finally
// {
//    try
//    {
//       datasrcDB.dropUsr( "test", "test" );
//    }
//    catch( e )
//    {
//       if( e != SDB_AUTH_USER_NOT_EXIST )
//       {
//          throw new Error( e );
//       }
//    }
//    try
//    {
//       datasrcDB.dropUsr( userName, passwd );
//    }
//    catch( e )
//    {
//       if( e != SDB_AUTH_USER_NOT_EXIST )
//       {
//          throw new Error( e );
//       }
//    }
//    finally
//    {
//       datasrcDB.close();
//    }
// }

function test ()
{
   datasrcDB.createUsr( userName, passwd );
   var csName = "cs_22853";
   var clName = "cl_22853";
   var srcCSName = "datasrcCS_22853";
   var dataSrcName = "datasrc22853";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dataSource = db.getDataSource( dataSrcName );
   var dbcs = commCreateCS( db, csName );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   datasrcDB.dropUsr( userName, passwd );
   datasrcDB.createUsr( "test", "test" );

   var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var dbcl = sdb.getCS( csName ).getCL( clName );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];
   assert.tryThrow( [SDB_AUTH_AUTHORITY_FORBIDDEN], function() 
   {
      dbcl.insert( docs );
   } );
   sdb.close();

   var dataSource = db.getDataSource( dataSrcName );
   dataSource.alter( { "User": "test", "Password": "test" } );

   var sdb = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   var dbcl = sdb.getCS( csName ).getCL( clName );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];
   dbcl.insert( docs );
   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   commCompareResults( cursor, docs );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   sdb.close();
}