/******************************************************************************
 * @Description   : seqDB-23792:dropCL后renameCS，恢复/强制恢复CL
 * @Author        : liuli
 * @CreateTime    : 2021.04.01
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName1 = "cs_23792_1";
   var csName2 = "cs_23792_2";
   var clName = "cl_23792";

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   cleanRecycleBin( db, "cs_23792" );

   var dbcs = commCreateCS( db, csName1 );
   var dbcl = dbcs.createCL( clName );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: 2 }, { a: 3, b: 3 }];
   dbcl.insert( docs );

   // 删除 cl 后 renameCS，在进行恢复
   dbcs.dropCL( clName );
   db.renameCS( csName1, csName2 );

   var recycleName = getOneRecycleName( db, csName1 + "." + clName, "Drop" );
   assert.tryThrow( SDB_RECYCLE_CONFLICT, function()
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 进行强制恢复
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );
   var dbcl = db.getCS( csName2 ).getCL( clName );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   cleanRecycleBin( db, "cs_23792" );
}