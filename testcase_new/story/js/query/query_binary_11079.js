/************************************
*@Description :  seqDB-11079：查询binary类型数据，参数错误
*@author      :  wangkexin 2019.6.6  huangxiaoni 2020.10.12
**************************************/
testConf.clName = COMMCLNAME + "_11079";

main( test );
function test ( arg )
{
   var cl = arg.testCL;
   cl.insert( { "a": { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } } );

   //查询binary类型数据，$type值错误
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( { a: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1000" } } ).toArray();
   } );

   //查询binary类型数据，缺少$type 
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.find( { bindata: { "$binary": "aGVsbG8gd29ybGQ=" } } ).toArray();
   } );
}