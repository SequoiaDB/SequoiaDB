/******************************************************************************
 * @Description   : seqDB-24107:使用数据源删除CL检查回收站
 * @Author        : liuli
 * @CreateTime    : 2021.04.21
 * @LastEditTime  : 2022.08.15
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc24107";
   var csName = "cs_24107";
   var clName = "cl_24107";
   var srcCSName = "datasrcCS_24107";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   cleanRecycleBin( db, csName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl );
   var dsVersion = getDSVersion( dataSrcName );

   if( dsVersion[0] >= 3 && dsVersion[1] >= 6 )
   {
      cleanRecycleBin( datasrcDB, srcCSName );
   }

   var dbcs = db.createCS( csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   dbcl.insert( { a: 1 } );

   // 删除CL后检查回收项目
   dbcs.dropCL( clName );
   var option = { "OriginName": csName + "." + clName };
   var recyclebinItem = db.getRecycleBin().list( option ).next();
   assert.equal( recyclebinItem, undefined, "Recycle bin property error" );

   // 删除CS后恢复CS项目
   db.dropCS( csName );
   var recycleName = getOneRecycleName( db, csName );
   db.getRecycleBin().returnItem( recycleName );
   checkRecycleItem( recycleName );

   // 恢复后检查，不存在使用数据源的CL
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      db.getCS( csName ).getCL( clName );
   } );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   cleanRecycleBin( db, csName );
   if( dsVersion[0] >= 3 && dsVersion[1] >= 6 )
   {
      cleanRecycleBin( datasrcDB, srcCSName );
   }
   datasrcDB.close();
}