/******************************************************************************
 * @Description   : seqDB-23234:创建复合索引且NotArray:false，对索引字段做数据操作
 * @Author        : Xiaoni Huang
 * @CreateTime    : 2021.01.09
 * @LastEditTime  : 2021.01.09
 * @LastEditors   : Xiaoni Huang
 ******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_cl_23234";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   // 创建索引   
   var notArray = true;
   var idxName = "idx";
   cl.createIndex( idxName, { "a.b": 1 }, { "NotArray": notArray } )
   // 检查索引NotArray属性正确性   
   var idxInfo = JSON.parse( cl.getIndex( idxName ).toString() );
   assert.equal( idxInfo.IndexDef.NotArray, notArray, "idxInfo = " + idxInfo );

   // 父字段为数组   
   assert.tryThrow( SDB_IXM_KEY_NOT_SUPPORT_ARRAY, function()
   {
      cl.insert( { "a": [1] } )
   } );
   var cnt = cl.count();
   assert.equal( cnt, 0 );

   // 父字段为数组嵌套对象
   assert.tryThrow( SDB_IXM_KEY_NOT_SUPPORT_ARRAY, function()
   {
      cl.insert( { "a": [{ "b": 1 }] } )
   } );
   var cnt = cl.count();
   assert.equal( cnt, 0 );

   // 子字段为数组   
   assert.tryThrow( SDB_IXM_KEY_NOT_SUPPORT_ARRAY, function()
   {
      cl.insert( { "a": { "b": [1] } } )
   } );
   var cnt = cl.count();
   assert.equal( cnt, 0 );

   // 子字段为数组嵌套对象
   assert.tryThrow( SDB_IXM_KEY_NOT_SUPPORT_ARRAY, function()
   {
      cl.insert( { "a": { "b": [{ "c": 1 }] } } )
   } );
   var cnt = cl.count();
   assert.equal( cnt, 0 );

   // 子字段为数组嵌套对象
   var records = [{ "a": { "b": { "c": [1] } } }, { "a": { "b": { "c": [1, { "d": [1, { "e": 1 }] }] } } }];
   cl.insert( records );
   var cursor = cl.find();
   commCompareResults( cursor, records );
}