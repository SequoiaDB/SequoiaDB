/******************************************************************************
 * @Description   : seqDB-23233:创建复合索引且NotArray:true，对索引字段做数据操作
 * @Author        : Xiaoni Huang
 * @CreateTime    : 2021.01.09
 * @LastEditTime  : 2021.01.09
 * @LastEditors   : Xiaoni Huang
 ******************************************************************************/
testConf.clName = CHANGEDPREFIX + "_cl_23233";

main( test );
function test ( testPara )
{
   var cl = testPara.testCL;
   // 创建索引
   var notArray = true;
   var idxName = "idx";
   cl.createIndex( idxName, { "a": 1, "b": 1 }, { "NotArray": notArray } )
   // 检查索引NotArray属性正确性   
   var idxInfo = JSON.parse( cl.getIndex( idxName ).toString() );
   assert.equal( idxInfo.IndexDef.NotArray, notArray, "idxInfo = " + idxInfo );

   // 增删改查数据，部分索引字段插入非数组数据，1个索引字段插入为数组数据
   var records = [{ "a": 1, "b": 1 }, { "a": 1, "b": 2 }];
   cl.insert( records );

   var record = { "a": 1, "b": [1] };
   assert.tryThrow( SDB_IXM_KEY_NOT_SUPPORT_ARRAY, function()
   {
      cl.insert( record )
   } );
   var cnt = cl.count( record );
   assert.equal( cnt, 0 );

   assert.tryThrow( SDB_IXM_KEY_NOT_SUPPORT_ARRAY, function()
   {
      cl.update( { "$set": record } )
   } );
   var cnt = cl.count( record );
   assert.equal( cnt, 0 );

   cl.remove( record );

   var cursor = cl.find();
   commCompareResults( cursor, records );
}