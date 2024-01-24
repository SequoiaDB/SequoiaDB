/******************************************************************************
 * @Description   : seqDB-23815:truncate后renameCL，恢复/强制恢复CL
 * @Author        : liuli
 * @CreateTime    : 2021.04.09
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_23815";
   var clName1 = "cl_23815_1";
   var clName2 = "cl_23815_2";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var dbcl = commCreateCL( db, csName, clName1 );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "tests2", c: "test" }, { a: 3, b: { b: 1 } }];
   dbcl.insert( docs );

   // truncate后renameCL
   dbcl.truncate();
   dbcs.renameCL( clName1, clName2 );

   // 恢复truncate项目
   var recycleName = getOneRecycleName( db, csName + "." + clName1, "Truncate" );
   assert.tryThrow( [SDB_RECYCLE_CONFLICT], function() 
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 强制恢复truncate项目
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );
   var dbcl = db.getCS( csName ).getCL( clName1 );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   // 校验clName2不存在
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcs.getCL( clName2 );
   } );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}