/************************************
*@Description : seqDB-11080：查询regex类型数据，参数错误 
*@author      : wangkexin 2019.6.6  huangxiaoni 2020.10.12
**************************************/
testConf.clName = COMMCLNAME + "_11080";

main( test );
function test ( arg )
{
   var cl = arg.testCL;

   //查询regex类型数据，$options值错误
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( { regex: { $regex: "aaa", $options: 1 } } ).toArray();
   } );

   //查询regex类型数据，带非 $options 的其他参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( { regex: { $regex: "aaa", a: "1" } } ).toArray();
   } );
}
