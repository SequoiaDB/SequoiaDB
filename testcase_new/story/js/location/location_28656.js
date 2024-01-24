/******************************************************************************
 * @Description   : seqDB-28656:使用setAttributes设置option参数校验
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.11.15
 * @LastEditTime  : 2022.11.18
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location = "location_28656";
   // 获取coord节点
   var coord = db.getCoordRG().getSlave();

   // 设置节点参数为非法值
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      coord.setAttributes( { GroupID: location } );
   } );
   checkNodeLocation( coord, undefined );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      coord.setAttributes( "location" );
   } );
   checkNodeLocation( coord, undefined );
}