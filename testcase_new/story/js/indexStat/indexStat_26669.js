/******************************************************************************
 * @Description   : seqDB-26669:js 驱动参数测试
 * @Author        : Lin Suqiang
 * @CreateTime    : 2022.07.21
 * @LastEditTime  : 2022.07.21
 * @LastEditors   : Lin Suqiang
 ******************************************************************************/
testConf.clName = "cl_26669";

main( test );

function test ()
{
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      testPara.testCL.getIndexStat( "indexName", "false" );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      testPara.testCL.getIndexStat( "indexName", null );
   } );
}

