/******************************************************************************
 * @Description   : seqDB-23789:恢复的项目在编目不存在
 * @Author        : liuli
 * @CreateTime    : 2021.04.14
 * @LastEditTime  : 2022.06.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_23789";
   var clName = "cl_23789";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   commCreateCL( db, csName, clName );

   // 删除CS
   db.dropCS( csName );

   // 删除catalog项目后恢复
   var recycleName = getOneRecycleName( db, csName, "Drop" );
   var cata = db.getCatalogRG().getMaster().connect();
   cata.getCS( "SYSRECYCLEBIN" ).getCL( "SYSCOLLECTIONSPACES" ).remove( { "Name": csName } );
   assert.tryThrow( [SDB_CAT_CORRUPTION], function() 
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   cata.close();
   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}