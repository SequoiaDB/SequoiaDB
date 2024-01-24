/******************************************************************************
 * @Description   : seqDB-23811:truncate后原集合新写入数据，恢复truncate
 * @Author        : liuli
 * @CreateTime    : 2021.04.08
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_23811";

main( test );
function test ( args )
{
   cleanRecycleBin( db, COMMCSNAME );;
   var dbcl = args.testCL;
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "tests2", c: "test" }, { a: 3, b: { b: 1 } }];
   dbcl.insert( docs );

   // CL执行truncate后删除CL写入新数据再恢复
   dbcl.truncate();
   dbcl.insert( { a: 1 } );
   var recycleName = getOneRecycleName( db, COMMCSNAME + "." + testConf.clName, "Truncate" );
   assert.tryThrow( [SDB_RECYCLE_CONFLICT], function() 
   {
      db.getRecycleBin().returnItem( recycleName );;
   } );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, [{ a: 1 }] );

   // 强制恢复truncate项目，新写入数据被覆盖
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );
}