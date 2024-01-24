/******************************************************************************
 * @Description   : seqDB-31022:存在大量索引，更新数据后执行count
 * @Author        : liuli
 * @CreateTime    : 2023.04.03
 * @LastEditTime  : 2023.04.03
 * @LastEditors   : liuli
 ******************************************************************************/

main( test );
function test ()
{
   var indexName = "index_31022_";
   var csName = "cs_31022";
   var clName1 = "cl_31022_1";
   var clName2 = "cl_31022_2";
   var clName3 = "cl_31022_3";

   commDropCS( db, csName );
   var dbcs = commCreateCS( db, csName );
   var groupNames = commGetDataGroupNames( db );
   var dbcl1 = dbcs.createCL( clName1, { Group: groupNames[0] } );
   var dbcl2 = dbcs.createCL( clName2, { Group: groupNames[0] } );
   var dbcl3 = dbcs.createCL( clName3, { Group: groupNames[0] } );

   var docs = [];
   var expRecs1 = [];
   var recordNum = 100 + parseInt( Math.random() * 500 );
   for( var i = 0; i < recordNum; i++ )
   {
      docs.push( { a: i } );
      expRecs1.push( { a: i + 1 } );
   }
   dbcl1.insert( docs );
   dbcl2.insert( docs );
   dbcl3.insert( docs );

   // 集合 dbcl1 创建 30 个索引
   for( var i = 0; i < 30; i++ )
   {
      var indexDef = {};
      indexDef["a" + i] = 1;
      dbcl1.createIndex( indexName + i, indexDef );
   }

   // dbcl1 更新数据
   dbcl1.update( { $inc: { a: 1 } } )

   // 校验 count 结果
   assert.equal( dbcl1.count(), recordNum );
   assert.equal( dbcl2.count(), recordNum );
   assert.equal( dbcl3.count(), recordNum );

   var cursor = dbcl1.find().sort( { a: 1 } );
   commCompareResults( cursor, expRecs1 );

   var cursor = dbcl2.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   var cursor = dbcl3.find().sort( { a: 1 } );
   commCompareResults( cursor, docs );

   commDropCS( db, csName );
}