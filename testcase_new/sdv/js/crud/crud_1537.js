/******************************************************************************
 * @Description   : seqDB-1537:多条记录+bulkInsert+不创建_id索引+压缩_basicOperation.sd.01.006
 * @Author        : Zhang Yanan
 * @CreateTime    : 2021.07.12
 * @LastEditTime  : 2021.07.12
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_1537";
testConf.clOpt = { AutoIndexId: false, Compressed: true }

main( test );

function test ( args )
{
   var varCL = args.testCL;
   // 插入1000条数据
   var insertCount1 = 1000
   for( var i = 0; i < insertCount1; i++ )
   {
      varCL.insert( { a: i, b: i, c: i } );
   }
   // 查询结果
   var rc = varCL.find();
   assert.equal( rc.count(), insertCount1 );

   // 更新记录报错
   assert.tryThrow( SDB_RTN_AUTOINDEXID_IS_FALSE, function()
   {
      varCL.update( { $inc: { b: 1 } } );
   } );
   // 删除记录报错
   assert.tryThrow( SDB_RTN_AUTOINDEXID_IS_FALSE, function()
   {
      varCL.remove( { a: { $gte: 100 } } );
   } );
   // 再次插入1000条数据
   var insertCount2 = 2000;
   for( var i = insertCount1; i < insertCount2; i++ )
   {
      varCL.insert( { a: i, b: i, c: i } );
   }
   // 查询结果  
   var rc = varCL.find();
   assert.equal( rc.count(), insertCount2 );

   var docs = [];
   for( i = 0; i < insertCount2; i++ ) 
   {
      docs.push( { a: i, b: i, c: i } );
   }

   var rc = varCL.find();
   commCompareResults( rc, docs );
}