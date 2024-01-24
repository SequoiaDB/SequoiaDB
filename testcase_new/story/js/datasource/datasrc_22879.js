/******************************************************************************
 * @Description   : seqDB-22879:源集群上创建cl，数据源上不存在映射cl
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22879";
   var csName = "cs_22879";
   var csName1 = "cs_22879a";
   var clName = "cl_22879";
   var srccsName = "srccs_22879";
   var srcclName = "srccl_22879";
   commDropCS( datasrcDB, srccsName );
   commDropCS( datasrcDB, csName );
   commDropCS( db, csName );
   commDropCS( db, csName1 );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, csName );
   commCreateCS( datasrcDB, srccsName );
   commCreateCL( datasrcDB, srccsName, srcclName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dsMarjorVersion = getDSMajorVersion( dataSrcName );

   //a、指定Mapping参数映射集合全名，数据源上不存在cs、cl
   var cs = db.createCS( csName );
   //3.0以上版本cs不存在报错-34   
   if( dsMarjorVersion < 3 )
   {
      assert.tryThrow( SDB_DMS_NOTEXIST, function()
      {
         cs.createCL( clName, { DataSource: dataSrcName, Mapping: "nocs.nocl" } );
      } );
   }
   else
   {
      println( "---version=" + dsMarjorVersion )
      assert.tryThrow( SDB_DMS_NOTEXIST, function()
      //assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
      {
         cs.createCL( clName, { DataSource: dataSrcName, Mapping: "nocs.nocl" } );
      } );
   }


   //b、指定Mapping参数映射集合全名，数据源上存在cs、不存在cl  
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      cs.createCL( clName, { DataSource: dataSrcName, Mapping: srccsName + ".nocl" } );
   } );

   //b、指定Mapping参数映射集合全名，数据源上不存在cs、存在cl  
   //3.0以上版本cs不存在报错-34   
   if( dsMarjorVersion < 3 )
   {
      assert.tryThrow( SDB_DMS_NOTEXIST, function()
      {
         cs.createCL( clName, { DataSource: dataSrcName, Mapping: "nocs." + srcclName } );
      } );
   }
   else
   {
      assert.tryThrow( SDB_DMS_NOTEXIST, function()
      {
         cs.createCL( clName, { DataSource: dataSrcName, Mapping: "nocs." + srcclName } );
      } );
   }

   //c、指定Mapping参数映射集合短名，数据源上不存在同名cs
   var cs1 = db.createCS( csName1 );
   //3.0以上版本cs不存在报错-34   
   if( dsMarjorVersion < 3 )
   {
      assert.tryThrow( SDB_DMS_NOTEXIST, function()
      {
         cs1.createCL( clName, { DataSource: dataSrcName, Mapping: srcclName } );
      } );
   }
   else
   {
      assert.tryThrow( SDB_DMS_NOTEXIST, function()
      {
         cs1.createCL( clName, { DataSource: dataSrcName, Mapping: srcclName } );
      } );
   }

   //c、指定Mapping参数映射集合短名，数据源上存在同名cs、不存在同名cl
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      cs.createCL( clName, { DataSource: dataSrcName, Mapping: clName } );
   } );

   //d、指定Mapping参数映射集合短名，数据源上存在同名cs不存在同名cl    
   if( dsMarjorVersion < 3 )
   {
      assert.tryThrow( SDB_DMS_NOTEXIST, function()
      {
         cs1.createCL( clName, { DataSource: dataSrcName, Mapping: "nocl" } );
      } );
   }
   else
   {
      assert.tryThrow( SDB_DMS_NOTEXIST, function()
      {
         cs1.createCL( clName, { DataSource: dataSrcName, Mapping: "nocl" } );
      } );
   }


   //e、不指定Mapping参数映射，不存在同名cs，存在同名cl 
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      cs.createCL( srcclName, { DataSource: dataSrcName } );
   } );

   //f、不指定Mapping参数映射，存在同名cs不存在同名cl 
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      cs.createCL( clName, { DataSource: dataSrcName } );
   } );

   //f、不指定Mapping参数映射，存在同名cs不存在同名cl 
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      cs.createCL( clName, { DataSource: dataSrcName } );
   } );

   db.dropCS( csName );
   db.dropCS( csName1 );
   datasrcDB.dropCS( srccsName );
   datasrcDB.dropCS( csName );
   db.dropDataSource( dataSrcName );
   datasrcDB.close();
}

