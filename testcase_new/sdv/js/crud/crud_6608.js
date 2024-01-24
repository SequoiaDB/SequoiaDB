/******************************************************************************
 * @Description   : seqDB-6608:使用BinData插入编码错误的二进制数据
 * @Author        : Zhang Yanan
 * @CreateTime    : 2021.08.09
 * @LastEditTime  : 2021.08.09
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_6608";

main( test );

function test ( args )
{
   var varCL = args.testCL;

   var docs = [];
   docs.push( { 'a': { $binary: 'aGVsbG8gd29ybGQ', $type: '1' } } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      varCL.insert( docs );
   } );

   var rc = varCL.find();
   var docs1 = [];
   commCompareResults( rc, docs1 );
}