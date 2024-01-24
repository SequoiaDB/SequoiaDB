/******************************************************************************
 * @Description   : seqDB-25480:dropCL后dropCS，先恢复dropCS在恢复dropCL 
 * @Author        : liuli
 * @CreateTime    : 2022.04.06
 * @LastEditTime  : 2022.04.06
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25480";
   var clName = "cl_25480";
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   // 创建CL并插入数据
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName );
   var docs = insertBulkData( dbcl, 1000 );

   // 执行truncate
   dbcl.truncate();

   // 删除CL
   dbcs.dropCL( clName );

   // 删除CS
   db.dropCS( csName );

   // 恢复dropCS项目
   var recycleName = getOneRecycleName( db, csName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );

   // 恢复dropCL项目
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );

   // 校验数据
   var dbcl = db.getCS( csName ).getCL( clName );
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, [] );

   // 恢复truncate项目
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Truncate" );
   assert.tryThrow( SDB_RECYCLE_CONFLICT, function()
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 强制恢复trucnate项目
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );

   // 校验数据
   var dbcl = db.getCS( csName ).getCL( clName );
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}