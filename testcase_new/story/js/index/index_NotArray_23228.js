/******************************************************************************
 * @Description   : seqDB-23228:创建/列取普通索引，NotArray属性测试
 * @Author        : Yi Pan
 * @CreateTime    : 2021.01.19
 * @LastEditTime  : 2021.01.19
 * @LastEditors   : Yi Pan
 ******************************************************************************/

testConf.clName = COMMCLNAME + "_23228";
var idxname = "idx23228";

main( test );

function test ()
{
   var cl = testPara.testCL;

   //默认方法创建name字段索引
   cl.createIndex( idxname, { 'name': 1 } );
   checkNotArray( cl, idxname, false );
   cl.dropIndex( idxname );

   //NotArray=false 创建name字段索引
   cl.createIndex( idxname, { 'name': 1 }, { NotArray: false } );
   checkNotArray( cl, idxname, false );

   //插入数据
   cl.insert( { 'name': 1 } );
   cl.insert( { 'name': [1] } );

   //查询结果
   var actResult = cl.find();
   var expResult = [{ 'name': 1 }, { 'name': [1] }];
   commCompareResults( actResult, expResult );
   cl.dropIndex( idxname );

   //NotArray=true 创建age字段索引
   cl.createIndex( idxname, { 'age': 1 }, { NotArray: true } );
   checkNotArray( cl, idxname, true );

   //插入数据
   cl.insert( { 'age': 3 } );
   assert.tryThrow( SDB_IXM_KEY_NOT_SUPPORT_ARRAY, function()
   {
      cl.insert( { 'age': [3] } );
   } );

   //查询结果
   actResult = cl.find();
   expResult = [{ 'name': 1 }, { 'name': [1] }, { 'age': 3 }];
   commCompareResults( actResult, expResult );
   cl.dropIndex( idxname );

}
