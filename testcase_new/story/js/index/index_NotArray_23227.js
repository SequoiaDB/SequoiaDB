/******************************************************************************
 * @Description   : seqDB-23227:创建/列取$id索引,NotArray属性测试
 * @Author        : Yi Pan
 * @CreateTime    : 2021.01.19
 * @LastEditTime  : 2021.01.19
 * @LastEditors   : Yi Pan
 ******************************************************************************/

testConf.clName = COMMCLNAME + "_23227";

main( test );

function test ()
{
   var cl = testPara.testCL;

   //自动生成id索引 检查NotArray是否正确
   checkNotArray( cl, "$id", true );

   //删除id索引 检查索引是否存在
   cl.dropIdIndex();
   assert.tryThrow( SDB_IXM_NOTEXIST, function()
   {
      cl.getIndex( "$id" );
   } );

   //创建id索引 检查NotArray是否正确
   cl.createIdIndex();
   checkNotArray( cl, "$id", true );

   //插入数据
   cl.insert( { _id: 1 } );
   //插入非法数据
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.insert( { _id: [1] } );
   } );

   //查询结果
   var actResult = cl.find();
   var expResult = [{ _id: 1 }];
   commCompareResults( actResult, expResult, false );

}


