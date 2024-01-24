/******************************************************************************
 * @Description   : seqDB-23791:dropCL后dropCS，恢复/强制恢复CL
 * @Author        : liuli
 * @CreateTime    : 2021.04.07
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_23791";
   var clName = "cl_23791";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: 2 }, { a: 3, b: 3 }];
   dbcl.insert( docs );

   // 删除 cl 后 删除 cs 再进行恢复
   dbcs.dropCL( clName );
   db.dropCS( csName );

   var recycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 进行强制恢复
   assert.tryThrow( SDB_DMS_CS_NOTEXIST, function()
   {
      db.getRecycleBin().returnItem( recycleName, { Enforced: true } );
   } );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}