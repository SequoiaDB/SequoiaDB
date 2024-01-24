/******************************************************************************
*@Description  seqDB-22758:dropCL参数校验
               1. dropCL 不存在的 cl
               2. dropCL 带 . 的 cl
               3. dropCL 长度为 128B 的 cl
               4. dropCL 空字符串的 cl
               5. dropCL 以 $ 开头的 cl
*@author      liyuanyue
*@createdate  2020.09.16
******************************************************************************/
testConf.csName = COMMCSNAME + "_22758";
main( test );

function test ( testPara )
{
   var cs = testPara.testCS;

   var notExistsClName = COMMCLNAME + "_notExists_22758";
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      cs.dropCL( notExistsClName );
   } );

   var withdotClName = COMMCLNAME + "_._22758";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cs.dropCL( withdotClName );
   } );

   var tooLong128BClName = COMMCLNAME + "_128B_22758";
   var len = tooLong128BClName.length;
   for( var i = 0; i < 128 - len; i++ )
   {
      tooLong128BClName += 'a';
   }
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cs.dropCL( tooLong128BClName );
   } );

   var emptyClName = "";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cs.dropCL( emptyClName );
   } );

   var with$ClName = "$" + COMMCLNAME + "_22758";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cs.dropCL( with$ClName );
   } );
}