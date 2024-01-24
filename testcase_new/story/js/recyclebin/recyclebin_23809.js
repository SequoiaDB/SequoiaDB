/******************************************************************************
 * @Description   : seqDB-23809:truncate后dropCL，恢复truncate
 * @Author        : liuli
 * @CreateTime    : 2021.04.07
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_23809";

main( test );
function test ( args )
{
   var originName = COMMCSNAME + "." + testConf.clName;

   cleanRecycleBin( db, COMMCSNAME );
   var dbcs = db.getCS( COMMCSNAME );
   var dbcl = args.testCL;
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "tests2", c: "test" }, { a: 3, b: { b: 1 } }];
   dbcl.insert( docs );

   // CL执行truncate后删除CL再恢复，并校验数据正确性
   dbcl.truncate();
   dbcs.dropCL( testConf.clName );
   var recycleName = getOneRecycleName( db, originName, "Truncate" );
   db.getRecycleBin().returnItem( recycleName );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   // 写入新的数据后强制恢复dropCL
   dbcl.insert( { a: 4, b: 2 } );
   var recycleName = getOneRecycleName( db, originName, "Drop" );
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, [] );
   checkRecycleItem( recycleName );
}