/******************************************************************************
 * @Description   : seqDB-23830:SkipRecycleBin参数校验
 * @Author        : Yang Qincheng
 * @CreateTime    : 2021.04.23
 * @LastEditTime  : 2022.08.15
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.csName = CHANGEDPREFIX + "_cs_23830";
testConf.clName = COMMCLNAME + "_23830";

main( test );
function test ()
{
   assert.equal( db.getRecycleBin().getDetail().toObj().Enable, true );

   var cl = testPara.testCL;

   cleanRecycleBin( db, testConf.csName );
   cl.insert( { "a": 1 } );

   // 无效参数值校验
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.truncate( { "SkipRecycleBin": 0 } )
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.truncate( { "SkipRecycleBin": 1 } )
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.truncate( { "SkipRecycleBin": -1 } )
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.truncate( { "SkipRecycleBin": null } )
   } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.truncate( { "SkipRecycleBin": "" } )
   } );
}