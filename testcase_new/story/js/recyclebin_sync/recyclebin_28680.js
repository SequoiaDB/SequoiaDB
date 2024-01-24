/******************************************************************************
 * @Description   : seqDB-28680:恢复回收站项目，编目快照中更新时间信息验证
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.11.09
 * @LastEditTime  : 2022.11.17
 * @LastEditors   : Xu Mingxing
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var csName = "cs_28680";
   var clName = "cl_28680";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = db.createCS( csName );
   var dbcl = dbcs.createCL( clName );
   dbcl.insert( { a: 1 } );

   // 查看编目快照信息
   var cursor = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + clName } );
   var preUpdateTime = cursor.current().toObj().UpdateTime;
   cursor.close();

   dbcl.truncate();
   // 恢复集合数据
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Truncate" );
   db.getRecycleBin().returnItem( recycleName );
   // 查看编目快照信息,更新后
   var cursor = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + clName } );
   var postUpdateTime = cursor.current().toObj().UpdateTime;
   cursor.close();
   // updateTime更新为当前恢复数据的时间
   if( postUpdateTime <= preUpdateTime )
   {
      throw new Error( "postUpdateTime: " + postUpdateTime + ", preUpdateTime: " + preUpdateTime + ", expected postUpdateTime to be more than preUpdateTime" );
   }

   dbcs.dropCL( clName );
   // 恢复集合
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );
   // 查看编目快照信息,更新后
   var cursor = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + clName } );
   var postUpdateTime = cursor.current().toObj().UpdateTime;
   cursor.close();
   // updateTime更新为当前恢复集合的时间 
   if( postUpdateTime <= preUpdateTime )
   {
      throw new Error( "postUpdateTime: " + postUpdateTime + ", preUpdateTime: " + preUpdateTime + ", expected postUpdateTime to be more than preUpdateTime" );
   }

   db.dropCS( csName );
   // 恢复集合空间
   var recycleName = getOneRecycleName( db, csName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );
   // 查看编目快照信息,更新后
   var cursor = db.snapshot( SDB_SNAP_CATALOG, { Name: csName + "." + clName } );
   var postUpdateTime = cursor.current().toObj().UpdateTime;
   cursor.close();
   // updateTime更新为当前恢复集合空间的时间
   if( postUpdateTime <= preUpdateTime )
   {
      throw new Error( "postUpdateTime: " + postUpdateTime + ", preUpdateTime: " + preUpdateTime + ", expected postUpdateTime to be more than preUpdateTime" );
   }

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}