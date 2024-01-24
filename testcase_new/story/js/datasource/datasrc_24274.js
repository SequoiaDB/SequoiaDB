/******************************************************************************
 * @Description   : seqDB-24274:修改使用数据源的集合名后，另一coord访问该集合
 * @Author        : liuli
 * @CreateTime    : 2021.06.21
 * @LastEditTime  : 2021.06.21
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var coordArr = getCoordUrl( db );
   if( coordArr.length < 2 )
   {
      return;
   }
   var db1 = new Sdb( coordArr[0] );
   var db2 = new Sdb( coordArr[1] );

   var dataSrcName = "datasrc24273";
   var csName = "cs_24273";
   var clName = "cl_24273";
   var clName2 = "cl_24274_2"
   var srcCSName = "datasrcCS_24273";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );

   commCreateCL( datasrcDB, srcCSName, clName );
   var dbcs = commCreateCS( db1, csName );
   db1.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var dbcl2 = db2.getCS( csName ).getCL( clName );

   db2.getCS( csName ).renameCL( clName, clName2 );

   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.insert( { a: 1 } );
   } );

   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl2.insert( { a: 1 } );
   } );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
   db1.close();
   db2.close();
}