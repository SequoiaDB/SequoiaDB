/******************************************************************************
 * @Description   : seqDB-23812:truncate后新写入数据再次truncate，先恢复truncate2再恢复truncate1
 * @Author        : liuli
 * @CreateTime    : 2021.04.08
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_23812";

main( test );
function test ( args )
{
   cleanRecycleBin( db, COMMCSNAME );

   var dbcl = args.testCL;
   var docs1 = [{ a: 1, b: 1 }, { a: 2, b: "tests2", c: "test" }, { a: 3, b: { b: 1 } }];
   dbcl.insert( docs1 );

   // truncate后写入新数据再恢复，恢复后新插入数据被覆盖
   dbcl.truncate();
   var docs2 = [{ a: 4 }, { a: 5, b: "b" }];
   dbcl.insert( docs2 );
   dbcl.truncate();

   var recycleNames = getRecycleName( db, COMMCSNAME + "." + testConf.clName, "Truncate" )

   // 恢复第二次truncate
   db.getRecycleBin().returnItem( recycleNames[1] );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs2 );
   checkRecycleItem( recycleNames[1] );

   // 恢复第一次truncate
   assert.tryThrow( [SDB_RECYCLE_CONFLICT], function() 
   {
      db.getRecycleBin().returnItem( recycleNames[0] );
   } );

   // 强制恢复第一次truncate
   db.getRecycleBin().returnItem( recycleNames[0], { Enforced: true } );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs1 );
   checkRecycleItem( recycleNames[0] );
}