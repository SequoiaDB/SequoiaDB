/******************************************************************************
 * @Description   : seqDB-24616:更新非索引字段
 *                : seqDB-24617:更新索引键字段
 * @Author        : liuli
 * @CreateTime    : 2021.11.08
 * @LastEditTime  : 2021.11.08
 * @LastEditors   : liuli
 ******************************************************************************/

main( test );
function test ()
{
   var clName1 = "cl_24616_1";
   var clName2 = "cl_24616_2";
   var indexName1 = "index_24616_1";
   var indexName2 = "index_24616_2";
   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );

   var dbcl1 = commCreateCL( db, COMMCSNAME, clName1 );
   var dbcl2 = commCreateCL( db, COMMCSNAME, clName2 );

   var docs = [];
   for( var i = 0; i < 2000; i++ )
   {
      docs.push( { a: i, b: i, c: i } );
   }
   dbcl1.insert( docs );
   dbcl2.insert( docs );

   // cl1创建单索引，cl2创建复合索引
   dbcl1.createIndex( indexName1, { a: 1 } );
   dbcl2.createIndex( indexName2, { a: 1, b: 1 } );

   // 更新非索引键字段
   dbcl1.update( { $inc: { c: 1 } } );
   dbcl2.update( { $inc: { c: 1 } } );

   var expResult = [];
   for( var i = 0; i < 2000; i++ )
   {
      expResult.push( { a: i, b: i, c: i + 1 } );
   }
   var actResult = dbcl1.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl2.find().sort( { a: 1 } );
   commCompareResults( actResult, expResult );

   // 更新索引键字段
   dbcl1.update( { $inc: { a: 1 } } );
   dbcl2.update( { $inc: { a: 1 } } );

   var expResult = [];
   for( var i = 0; i < 2000; i++ )
   {
      expResult.push( { a: i + 1, b: i, c: i + 1 } );
   }
   var actResult = dbcl1.find().sort( { b: 1 } );
   commCompareResults( actResult, expResult );
   var actResult = dbcl2.find().sort( { b: 1 } );
   commCompareResults( actResult, expResult );

   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
}