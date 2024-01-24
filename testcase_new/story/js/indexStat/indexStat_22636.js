/***************************************************************************
@Description : seqDB-22636: 获取集合索引统计信息接口参数验证 
@Modify list : Zhao Xiaoni 2020/8/19
****************************************************************************/
testConf.clName = "cl_22636";

main( test );

function test( testPara )
{
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      testPara.testCL.getIndexStat();
   });
   assert.tryThrow( SDB_IXM_STAT_NOTEXIST, function()
   {  
      testPara.testCL.getIndexStat( "indexName" );
   });
   assert.tryThrow( SDB_INVALIDARG, function()
   {  
      testPara.testCL.getIndexStat( { "indexName": 1 } );
   });
}


