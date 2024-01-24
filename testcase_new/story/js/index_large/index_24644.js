/******************************************************************************
 * @Description   : seqDB-24644:创建唯一索引，删除插入索引记录
 * @Author        : liuli
 * @CreateTime    : 2021.11.17
 * @LastEditTime  : 2021.11.29
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24644";

main( test );
function test ( args )
{
   var dbcl = args.testCL;
   var indexName = "index_24644";

   dbcl.createIndex( indexName, { a: 1, b: 1 }, true );

   var arr = new Array( 550 );
   var testb = arr.join( '1' );
   var arr = new Array( 50 );
   var testc = arr.join( '1' );
   var docs = [];
   var insertNum = 100000;
   for( var i = 0; i < insertNum; i++ )
   {
      docs.push( { a: i, b: testb, c: testc } )
   }
   dbcl.insert( docs );

   var unusedValue = 89999;
   var removeEndNo = 99899;
   for( var i = 0; i < unusedValue; i++ )
   {
      dbcl.remove( { a: i } );
   }

   for( var i = unusedValue + 1; i < removeEndNo; i++ )
   {
      dbcl.remove( { a: i } );
   }

   dbcl.remove( { a: unusedValue } );

   dbcl.insert( { a: 89999, b: testb, c: testc, d: testb } );
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      dbcl.insert( { a: 89999, b: testb } );
   } );
}