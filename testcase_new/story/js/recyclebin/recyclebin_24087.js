/******************************************************************************
 * @Description   : seqDB-24087:dropCL后dropCS，创建同名CS，恢复/强制恢复CL
 * @Author        : liuli
 * @CreateTime    : 2021.04.14
 * @LastEditTime  : 2022.06.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_24087";
   var clName = "cl_24087";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   dbcs.createCL( clName );

   // 删除CL后删除CS，恢复CL项目
   dbcs.dropCL( clName );
   db.dropCS( csName );

   // 创建同名CS
   db.createCS( csName );

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