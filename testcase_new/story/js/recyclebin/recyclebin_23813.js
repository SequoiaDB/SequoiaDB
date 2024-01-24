/******************************************************************************
 * @Description   : seqDB-23813:truncate后dropCL，创建同名CL，恢复/强制恢复truncate
 * @Author        : liuli
 * @CreateTime    : 2021.04.08
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_23813";
   var clName = "cl_23813";
   var originName = csName + "." + clName;

   commDropCS( db, csName );
   cleanRecycleBin( db, COMMCSNAME );

   var dbcs = commCreateCS( db, csName );
   var dbcl = commCreateCL( db, csName, clName );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "tests2", c: "test" }, { a: 3, b: { b: 1 } }];
   dbcl.insert( docs );

   // truncate后删除CL，再创建同名CL
   dbcl.truncate();
   dbcs.dropCL( clName );
   dbcs.createCL( clName );

   // 恢复truncate项目
   var recycleName = getOneRecycleName( db, originName, "Truncate" );
   assert.tryThrow( [SDB_RECYCLE_CONFLICT], function() 
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 强制恢复truncate项目
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );
   var cursor = dbcl.find();
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}