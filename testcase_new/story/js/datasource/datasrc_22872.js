/******************************************************************************
 * @Description   : seqDB-22872:数据源上删除已建立映射关系的cs
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc22872";
   var csName = "cs_22872";
   var srcCSName = "datasrcCS_22872";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, srcCLName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dsMarjorVersion = getDSMajorVersion( dataSrcName );
   db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );

   var cl = db.getCS( csName ).getCL( srcCLName );
   datasrcDB.dropCS( srcCSName );

   //2.8版本上cs不存在报错为23，该问题单不修复(参考问题单SEQUOIADBMAINSTREAM-6431）
   if( dsMarjorVersion > 2 )
   {
      assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
      {
         cl.find().toArray();
      } );
   }
   else
   {
      assert.tryThrow( SDB_DMS_NOTEXIST, function()
      {
         cl.find().toArray();
      } );
   }

   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}
