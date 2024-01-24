/******************************************************************************
 * @Description   : seqDB-23810:truncate后dropCL，恢复truncate，强制恢复dropCL
 * @Author        : liuli
 * @CreateTime    : 2021.04.16
 * @LastEditTime  : 2022.06.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_23810";

main( test );
function test ( args )
{
   var originName = COMMCSNAME + "." + testConf.clName;

   cleanRecycleBin( db, COMMCSNAME );
   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = args.testCL;
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "tests2", c: "test" }, { a: 3, b: { b: 1 } }];
   dbcl.insert( docs );

   // CL执行truncate后写入新数据删除CL
   dbcl.truncate();
   var docs2 = [{ a: 4 }, { a: 5, b: "b" }];
   dbcl.insert( docs2 );
   dbcs.dropCL( testConf.clName );

   // 恢复truncate项目，并校验数据正确性
   var recycleName = getOneRecycleName( db, originName, "Truncate" );
   db.getRecycleBin().returnItem( recycleName );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   // 恢复dropCL项目
   var recycleName = getOneRecycleName( db, originName, "Drop" );
   assert.tryThrow( [SDB_RECYCLE_CONFLICT], function() 
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 强制恢复dropCL项目
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs2 );
   checkRecycleItem( recycleName );
}