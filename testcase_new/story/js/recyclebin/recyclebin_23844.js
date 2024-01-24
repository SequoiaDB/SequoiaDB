/******************************************************************************
 * @Description   : seqDB-23844:clear清理回收站某个项目
 * @Author        : liuli
 * @CreateTime    : 2021.04.19
 * @LastEditTime  : 2022.04.13
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_23844";
   var clName = "cl_23844";
   var cata = db.getCatalogRG().getMaster().connect();

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   commCreateCS( db, csName );
   var dbcl = commCreateCL( db, csName, clName );

   // 执行truncate后，删除CS
   dbcl.insert( { a: 1 } );
   dbcl.truncate();
   db.dropCS( csName );

   // 删除dropCS项目
   var recycleName1 = getOneRecycleName( db, csName + "." + clName, "Truncate" );
   var recycleName2 = getOneRecycleName( db, csName, "Drop" );
   // 删除不存在的项目
   assert.tryThrow( [SDB_RECYCLE_CONFLICT], function() 
   {
      db.getRecycleBin().dropItem( recycleName2 );
   } );
   db.getRecycleBin().dropItem( recycleName2, true );
   checkRecycleItem( recycleName2 );

   // 删除dropCS项目后truncate项目也被删除
   checkRecycleItem( recycleName1 );

   // 删除不存在的项目
   assert.tryThrow( [SDB_RECYCLE_ITEM_NOTEXIST], function() 
   {
      db.getRecycleBin().dropItem( recycleName1 );
   } );

   // 检查dropCS项目catalog对应回收站项目被删除
   var cursor = cata.getCS( "SYSRECYCLEBIN" ).getCL( "SYSRECYCLEITEMS" ).find( { "RecycleName": recycleName2 } );
   commCompareResults( cursor, [] );
   var cursor = cata.getCS( "SYSRECYCLEBIN" ).getCL( "SYSCOLLECTIONSPACES" ).find( { "Name": csName } );
   commCompareResults( cursor, [] );

   // 检查truncate项目catalog对应回收站项目被删除
   var cursor = cata.getCS( "SYSRECYCLEBIN" ).getCL( "SYSRECYCLEITEMS" ).find( { "RecycleName": recycleName1 } );
   commCompareResults( cursor, [] );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}