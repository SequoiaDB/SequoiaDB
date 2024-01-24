/******************************************************************************
 * @Description   : seqDB-26602:普通表只指定Update更新规则，单条插入冲突记录
 * @Author        : Lin Yingting
 * @CreateTime    : 2022.06.09
 * @LastEditTime  : 2022.06.09
 * @LastEditors   : Lin Yingting
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_26602";

main( test );

function test ( testPara )
{
   //创建普通表，创建唯一索引，插入记录
   var cl = testPara.testCL;
   cl.createIndex( "idx_a", { a: 1 }, true, true );
   cl.insert( { a: 1, b: 1 } );

   //普通表只指定更新规则，不指定UpdateOnDup，单条插入冲突记录
   assert.tryThrow( SDB_IXM_DUP_KEY, function ()
   {
      cl.insert( { a: 1, b: 2 }, { Update: { "$inc": { "a": 1 } } } );
   } );

   //检查表数据
   var actRes = cl.find().sort( { a: 1 } );
   var expRes = [{ a: 1, b: 1 }];
   commCompareResults( actRes, expRes );
}
