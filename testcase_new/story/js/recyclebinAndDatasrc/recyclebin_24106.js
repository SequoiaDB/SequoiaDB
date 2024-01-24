/******************************************************************************
 * @Description   : seqDB-24106:使用数据源删除CS检查回收站
 * @Author        : liuli
 * @CreateTime    : 2021.04.21
 * @LastEditTime  : 2022.08.15
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc24106";
   var csName = "cs_24106";
   var clName = "cl_24106";
   var srcCSName = "datasrcCS_24106";

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

   var dbcs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   dbcs.getCL( clName ).insert( { a: 1 } );

   db.dropCS( csName );
   var option = { "OriginName": csName };
   var recyclebinItem = db.getRecycleBin().list( option ).next();
   assert.equal( recyclebinItem, undefined, "Recycle bin property error" );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   cleanRecycleBin( db, csName );
   if( dsVersion[0] >= 3 && dsVersion[1] >= 6 )
   {
      cleanRecycleBin( datasrcDB, srcCSName );
   }
   datasrcDB.close();
}