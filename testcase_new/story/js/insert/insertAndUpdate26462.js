/******************************************************************************
 * @Description   : seqDB-26462:普通表单条插入冲突记录，更新后记录冲突
 * @Author        : Lin Yingting
 * @CreateTime    : 2022.05.13
 * @LastEditTime  : 2022.05.13
 * @LastEditors   : Lin Yingting
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_26462";

main( test );

function test ( testPara )
{
   //普通表创建唯一索引，插入记录
   var cl = testPara.testCL;
   cl.createIndex( "idx_a", { a: 1 }, true, true );
   cl.insert( [{ a: 1, b: 1 }, { a: 2, b: 2 }] );

   //指定UpdateOnDup和更新规则，插入冲突记录，更新后记录也冲突
   assert.tryThrow( SDB_IXM_DUP_KEY, function ()
   {
      cl.insert( { a: 1 }, { UpdateOnDup: true, Update: { "$inc": { "a": 1 } } } );
   } );
  
   //检查表数据
   var actRes = cl.find().sort( { a: 1 } );
   var expRes = [{ a: 1, b: 1 }, { a: 2, b: 2 }];
   commCompareResults( actRes, expRes );
}
