/******************************************************************************
*@Description : test Oma function: setIniConfigs getIniConfigs                                       
*               TestLink: seqDB-17868:sdb和sdbcm的setIniConfigs支持ini格式，ini字段数据类型测试 
*@Author      : 2019-3-7  XiaoNi Huang
*@Info		  : invalid type[null/array/......]
******************************************************************************/

main( test );

function test ()
{
   var filePath = WORKDIR + "/" + "config17973_sdbcm.conf";

   // sdb test
   // invalid type[null]
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      Oma.setIniConfigs( { "inv.null": null }, filePath );
   } );

   // invalid type[array]
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      Oma.setIniConfigs( { "inv.null": [1, 2] }, filePath );
   } );


   // sdbcm test
   var oma = new Oma( COORDHOSTNAME, CMSVCNAME );

   // invalid type[null]
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      oma.setIniConfigs( { "inv.null": null }, filePath );
   } );


   // invalid type[array]
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      oma.setIniConfigs( { "inv.null": [1, 2] }, filePath );
   } );

   oma.close();

}