/******************************************************************************
 * @Description   : seqDB-25381:设置AutoDrop为false，回收站相同项目大于MaxVersionNum 
 * @Author        : liuli
 * @CreateTime    : 2022.02.23
 * @LastEditTime  : 2022.03.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_25381";
   var clName = "cl_25381";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );

   // CL连续执行两次truncate
   dbcl.insert( { a: 1 } );
   dbcl.truncate();
   dbcl.insert( { a: 1 } );
   dbcl.truncate();

   // CL再次执行truncate
   assert.tryThrow( [SDB_RECYCLE_FULL], function() 
   {
      dbcl.truncate();
   } );

   // 删除CL
   assert.tryThrow( [SDB_RECYCLE_FULL], function() 
   {
      dbcs.dropCL( clName );
   } );

   // 清理CL对应的所有回收站项目
   cleanRecycleBin( db, csName + "." + clName );

   // 删除CL后创建同名CL再次删除，重复两次
   dbcs.dropCL( clName );
   dbcs.createCL( clName );
   dbcs.dropCL( clName );
   dbcs.createCL( clName );

   // 再次删除CL
   assert.tryThrow( [SDB_RECYCLE_FULL], function() 
   {
      dbcs.dropCL( clName );
   } );

   // 删除CS后创建同名CS再次删除，重复两次
   db.dropCS( csName );
   db.createCS( csName );
   db.dropCS( csName );
   db.createCS( csName );

   // 再次删除CS
   assert.tryThrow( [SDB_RECYCLE_FULL], function() 
   {
      db.dropCS( csName );
   } );

   // 指定SkipRecycleBin为true删除CS
   db.dropCS( csName, { SkipRecycleBin: true } );
   cleanRecycleBin( db, csName );
}