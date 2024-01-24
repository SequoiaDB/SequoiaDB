/************************************
*@Description : seqDB-5298:setSessionAttr，字段值超过边界值
*@author      : wangkexin 2019.6.5  huangxiaoni 2020.10.12
**************************************/
testConf.skipStandAlone = true;

main( test );
function test ( arg )
{
   // PreferedInstance字段值为0
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.setSessionAttr( { "PreferedInstance": 0 } );
   } );

   // PreferedInstance字段值为256
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.setSessionAttr( { "PreferedInstance": 256 } );
   } );

   // PreferedInstance字段值为字符串
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.setSessionAttr( { "PreferedInstance": "y" } );
   } );
}