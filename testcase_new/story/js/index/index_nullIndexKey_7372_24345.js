/******************************************************************************
 * @Description   : seqDB-7372:创建索引，其中索引键为空值
 *                  seqDB-24345:getIndex操作指定参数为空
 * @Author        : xiaojun Hu
 * @CreateTime    : 2014.05.20
 * @LastEditTime  : 2021.09.13
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24345_7372";
main( test );

function test ( args )
{
   var varCL = args.testCL;

   varCL.insert( { a: 1 } );
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      varCL.createIndex( "testindex", {}, false, false );
   } )

   assert.tryThrow( SDB_IXM_NOTEXIST, function()
   {
      varCL.getIndex( "testindex" )
   } )

   //getIndex操作指定参数为空
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      varCL.getIndex();
   } )
}

