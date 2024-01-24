/******************************************************************************
 * @Description   : seqDB-23788:恢复不存在的项目
 * @Author        : liuli
 * @CreateTime    : 2021.04.07
 * @LastEditTime  : 2021.04.07
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var recycleName = "noRecycle_23788";

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.getRecycleBin().returnItem( recycleName );
   } );
}