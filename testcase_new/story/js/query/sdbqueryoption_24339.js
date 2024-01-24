/******************************************************************************
 * @Description   : seqDB-24339:指定skip、limit、fiags为空查询记录
 * @Author        : Zhang Yanan
 * @CreateTime    : 2021.09.08
 * @LastEditTime  : 2021.09.16
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24339";

main( test );

function test ( args )
{
   var cl = args.testCL;

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( new SdbQueryOption().skip() ).toObj();
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( new SdbQueryOption().limit() ).toObj();
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( new SdbQueryOption().flags() ).toObj();
   } )
}