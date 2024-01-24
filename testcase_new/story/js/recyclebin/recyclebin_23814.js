/******************************************************************************
 * @Description   : seqDB-23814:truncate后dropCL，创建同名CL，写入新的数据，恢复truncate
 * @Author        : liuli
 * @CreateTime    : 2021.04.08
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_23814";
   var clName = "cl_23814";
   var originName = csName + "." + clName;

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = commCreateCL( db, csName, clName );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "tests2", c: "test" }, { a: 3, b: { b: 1 } }];
   dbcl.insert( docs );

   // truncate后删除CL，再创建同名CL并插入数据
   dbcl.truncate();
   dbcs.dropCL( clName );
   var dbcl2 = dbcs.createCL( clName );
   dbcl2.insert( [{ a: 4 }, { a: 5, b: "b" }] );

   // 恢复truncate项目
   var recycleName = getOneRecycleName( db, originName, "Truncate" );
   assert.tryThrow( [SDB_RECYCLE_CONFLICT], function() 
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 强制恢复truncate项目
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}