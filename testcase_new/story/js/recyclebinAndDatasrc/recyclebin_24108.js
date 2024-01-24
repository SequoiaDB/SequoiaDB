/******************************************************************************
 * @Description   : seqDB-24108:数据源有回收站功能，CS级别映射执行truncate
 * @Author        : liuli
 * @CreateTime    : 2021.04.21
 * @LastEditTime  : 2022.08.15
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc24108";
   var csName = "cs_24108";
   var clName = "cl_24108";
   var srcCSName = "datasrcCS_24108";
   var originName = srcCSName + "." + clName;

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
   var dbcl = dbcs.getCL( clName );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "tests2", c: "test" }, { a: 3, b: { b: 1 } }];
   dbcl.insert( docs );

   // 本地集群CL执行truncate，检查回收站
   dbcl.truncate();
   var option = { "OriginName": csName + "." + clName, "OpType": "Truncate" };
   var recyclebinItem = db.getRecycleBin().count( option );
   assert.equal( recyclebinItem, 0, "Recycle bin property error" );

   // 数据源端回收站恢复truncate项目
   if( dsVersion[0] >= 3 && dsVersion[1] >= 6 && isRecycleBinOpen( datasrcDB ) )
   {
      var recycleName = getOneRecycleName( datasrcDB, originName, "Truncate" );
      datasrcDB.getRecycleBin().returnItem( recycleName );
      var cursor = dbcl.find().sort( { a: 1 } );
      commCompareResults( cursor, docs );
   }
   else
   {
      var cursor = dbcl.find().sort( { a: 1 } );
      commCompareResults( cursor, [] );
   }

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   cleanRecycleBin( db, csName );
   if( dsVersion[0] >= 3 && dsVersion[1] >= 6 )
   {
      cleanRecycleBin( datasrcDB, srcCSName );
   }
   datasrcDB.close();
}